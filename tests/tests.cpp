#include "cli/args.hpp"
#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "ecc/reed_solomon.hpp"
#include "file/file.hpp"
#include "util/endian.hpp"
#include "video/header.hpp"
#include "video/video.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <initializer_list>
#include <numeric>
#include <print>
#include <random>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <opencv2/opencv.hpp>

using namespace ftv;

// a tiny harness: every failed check is printed and the exit code is the
// number of failures

namespace
{

int failures = 0;

void
check (bool condition, std::string_view what,
       std::source_location loc = std::source_location::current ())
{
  if (!condition)
    {
      ++failures;
      std::println (stderr, "FAIL {}:{}: {}", loc.file_name (), loc.line (),
                    what);
    }
}

std::vector<std::byte>
random_bytes (std::mt19937 &rng, std::size_t count)
{
  std::uniform_int_distribution<int> dist (0, 255);
  std::vector<std::byte> bytes (count);
  for (auto &b : bytes)
    {
      b = static_cast<std::byte> (dist (rng));
    }
  return bytes;
}

// flips count distinct bytes to a different value
void
corrupt (std::mt19937 &rng, std::span<std::byte> data, std::size_t count)
{
  std::vector<std::size_t> positions (data.size ());
  std::iota (positions.begin (), positions.end (), 0UZ);
  std::ranges::shuffle (positions, rng);
  std::uniform_int_distribution<int> dist (1, 255);
  for (std::size_t i = 0; i < count; ++i)
    {
      data[positions[i]] ^= static_cast<std::byte> (dist (rng));
    }
}

void
test_reed_solomon (std::mt19937 &rng)
{
  for (std::size_t trial = 0; trial < 300; ++trial)
    {
      const auto message = random_bytes (rng, 1 + trial % RS_DATA_SIZE);
      auto codeword = rs_encode_block (message);
      check (codeword.size () == message.size () + RS_PARITY_SIZE,
             "codeword size");

      const std::size_t errors = trial % (RS_PARITY_SIZE / 2 + 1);
      corrupt (rng, codeword, errors);
      const auto decoded = rs_decode_block (codeword);
      check (decoded && *decoded == message,
             std::format ("corrects {} errors in a {} byte codeword", errors,
                          codeword.size ()));
    }

  // beyond capacity: must not crash, and mostly reports failure
  std::size_t detected = 0;
  for (std::size_t trial = 0; trial < 50; ++trial)
    {
      const auto message = random_bytes (rng, RS_DATA_SIZE);
      auto codeword = rs_encode_block (message);
      corrupt (rng, codeword, 40);
      const auto decoded = rs_decode_block (codeword);
      if (!decoded || *decoded != message)
        {
          ++detected;
        }
    }
  check (detected == 50, "too many errors are never silently 'fixed' to the "
                         "original");

  // interleaving spreads a burst over all codewords
  const auto data = random_bytes (rng, 5000);
  auto encoded = rs_encode (data);
  check (encoded.size () == rs_encoded_size (data.size ()), "encoded size");
  const std::size_t codewords = encoded.size () / RS_CODEWORD_SIZE;
  const std::size_t burst = codewords * (RS_PARITY_SIZE / 2);
  for (std::size_t i = 1000; i < 1000 + burst; ++i)
    {
      encoded[i] = ~encoded[i];
    }
  const auto decoded = rs_decode (encoded, data.size ());
  check (decoded && *decoded == data, "interleaved burst is corrected");

  check (!rs_decode (std::span (encoded).first (100), data.size ()),
         "wrong encoded size is rejected");
}

void
test_header (std::mt19937 &rng)
{
  const header original{ 4, 123456789 };
  auto encoded = encode_header (original);
  check (encoded.size () == ENCODED_HEADER_SIZE, "encoded header size");

  auto decoded = decode_header (encoded);
  check (decoded && decoded->block_size == 4
             && decoded->payload_size == 123456789,
         "header round trip");

  corrupt (rng, encoded, RS_PARITY_SIZE / 2);
  decoded = decode_header (encoded);
  check (decoded && decoded->payload_size == 123456789,
         "damaged header is corrected");

  check (!decode_header (random_bytes (rng, ENCODED_HEADER_SIZE)),
         "random bytes are not a header");
  check (!decode_header (encode_header ({ 3, 10 })),
         "unsupported block size is rejected");
  check (!decode_header (encode_header ({ 4, MAX_PAYLOAD_SIZE + 1 })),
         "absurd payload size is rejected");
}

void
test_serialize (std::mt19937 &rng)
{
  const encrypted_data data{ random_bytes (rng, 100),
                             random_bytes (rng, AES_GCM_IV_SIZE),
                             random_bytes (rng, AES_GCM_TAG_SIZE),
                             random_bytes (rng, SALT_SIZE) };
  const auto serialized = serialize_encrypted_data (data);
  check (serialized.has_value (), "serialize");

  const auto back = deserialize_encrypted_data (*serialized);
  check (back && back->ciphertext () == data.ciphertext ()
             && back->init_vec () == data.init_vec ()
             && back->tag () == data.tag () && back->salt () == data.salt (),
         "serialize round trip");

  check (!deserialize_encrypted_data (std::span (*serialized).first (30)),
         "truncated data is rejected");
  check (!deserialize_encrypted_data ({}), "empty data is rejected");

  const encrypted_data bad_iv{ random_bytes (rng, 10), random_bytes (rng, 5),
                               random_bytes (rng, AES_GCM_TAG_SIZE),
                               random_bytes (rng, SALT_SIZE) };
  check (!serialize_encrypted_data (bad_iv), "wrong iv size is rejected");
}

std::vector<std::byte>
payload_with_name (std::string_view name)
{
  std::vector<std::byte> payload{};
  put_le (payload, static_cast<std::uint16_t> (name.size ()));
  for (const char c : name)
    {
      payload.push_back (static_cast<std::byte> (c));
    }
  payload.push_back (std::byte{ 'x' });
  return payload;
}

void
test_payload (std::mt19937 &rng)
{
  const file original{ "some/dir/report.pdf", random_bytes (rng, 500) };
  const auto restored = unpack_file (pack_file (original));
  check (restored && restored->path () == "report.pdf"
             && restored->data () == original.data (),
         "payload keeps only the file name and the contents");

  check (unpack_file (payload_with_name ("notes.txt")).has_value (),
         "plain name is accepted");
  for (const auto name :
       { "../evil", "/etc/passwd", "a/b", "..", ".", "", "dir/" })
    {
      check (!unpack_file (payload_with_name (name)),
             std::format ("name '{}' is rejected", name));
    }

  auto truncated = payload_with_name ("notes.txt");
  truncated.resize (5);
  check (!unpack_file (truncated), "truncated payload is rejected");
}

void
test_crypto (std::mt19937 &rng)
{
  const auto salt = random_bytes (rng, SALT_SIZE);
  const secure_key a{ "password", salt };
  const secure_key b{ "password", salt };
  const secure_key other_password{ "Password", salt };
  const secure_key other_salt{ "password", random_bytes (rng, SALT_SIZE) };
  check (std::ranges::equal (a.get (), b.get ()), "kdf is deterministic");
  check (!std::ranges::equal (a.get (), other_password.get ()),
         "different password gives a different key");
  check (!std::ranges::equal (a.get (), other_salt.get ()),
         "different salt gives a different key");

  try
    {
      const secure_key empty{ "" };
      check (false, "empty password throws");
    }
  catch (const std::invalid_argument &)
    {
    }

  const auto plaintext = random_bytes (rng, 1000);
  const auto encrypted = encrypt (plaintext, "password");
  check (encrypted.has_value (), "encrypt");

  const auto decrypted = decrypt (*encrypted, "password");
  check (decrypted && *decrypted == plaintext, "encrypt round trip");
  check (!decrypt (*encrypted, "wrong"), "wrong password is rejected");

  auto tampered_ciphertext = encrypted->ciphertext ();
  tampered_ciphertext[10] ^= std::byte{ 1 };
  const encrypted_data tampered{ tampered_ciphertext, encrypted->init_vec (),
                                 encrypted->tag (), encrypted->salt () };
  check (!decrypt (tampered, "password"), "tampered ciphertext is rejected");

  check (encrypt ({}, "password").has_value (), "empty data can be encrypted");
}

std::expected<parameters, std::string>
parse (std::initializer_list<std::string_view> args)
{
  const std::vector<std::string_view> list (args);
  return parse_arguments (list);
}

void
test_cli ()
{
  const auto full
      = parse ({ "encrypt", "in.txt", "-o", "out.avi", "-k", "pw", "-w", "640",
                 "-H", "480", "-f", "24", "-b", "8" });
  check (full && full->cmd == command::encrypt && full->input_file == "in.txt"
             && full->output_file == "out.avi" && full->key == "pw"
             && full->width == 640 && full->height == 480 && full->fps == 24
             && full->block_size == 8,
         "all options are parsed");

  const auto long_names
      = parse ({ "decrypt", "v.avi", "--key", "pw", "--output", "out.bin" });
  check (long_names && long_names->cmd == command::decrypt
             && long_names->output_file == "out.bin",
         "long option names");

  check (parse ({ "-h" }) && parse ({ "-h" })->help, "-h shows help");
  check (parse ({ "encrypt", "in", "--help" })
             && parse ({ "encrypt", "in", "--help" })->help,
         "--help after the command shows help");

  const auto dash_key = parse ({ "decrypt", "v.avi", "-k", "-h" });
  check (dash_key && !dash_key->help && dash_key->key == "-h",
         "an option value may start with '-'");

  check (parse ({ "decrypt", "v.avi", "-k", "pw" }).has_value (),
         "decrypt does not need -o");
  check (!parse ({}), "missing command");
  check (!parse ({ "encrpyt", "in", "-o", "o", "-k", "pw" }),
         "unknown command is rejected");
  check (!parse ({ "encrypt", "in", "-k", "pw" }), "encrypt needs -o");
  check (!parse ({ "encrypt", "in", "-o", "o" }), "key is required");
  check (!parse ({ "decrypt", "-k", "pw" }), "input is required");
  check (!parse ({ "decrypt", "in", "-k" }), "missing option value");
  check (!parse ({ "decrypt", "in", "-k", "pw", "--bogus" }),
         "unknown option");
  check (!parse ({ "decrypt", "in", "extra", "-k", "pw" }),
         "extra positional argument");
  for (const auto width : { "abc", "-5", "640px", "", "99", "4097" })
    {
      check (!parse ({ "encrypt", "in", "-o", "o", "-k", "pw", "-w", width }),
             std::format ("width '{}' is rejected", width));
    }
  check (!parse ({ "encrypt", "in", "-o", "o", "-k", "pw", "-f", "0" }),
         "fps 0 is rejected");
  check (!parse ({ "encrypt", "in", "-o", "o", "-k", "pw", "-b", "3" }),
         "block size 3 is rejected");
}

// redraws every frame of a video with a few random squares painted over it
void
damage_video (std::mt19937 &rng, const std::filesystem::path &from,
              const std::filesystem::path &to, std::size_t squares_per_frame)
{
  cv::VideoCapture in (from.string ());
  const auto width
      = static_cast<std::int32_t> (in.get (cv::CAP_PROP_FRAME_WIDTH));
  const auto height
      = static_cast<std::int32_t> (in.get (cv::CAP_PROP_FRAME_HEIGHT));
  cv::VideoWriter out (to.string (),
                       cv::VideoWriter::fourcc ('M', 'J', 'P', 'G'), 30,
                       cv::Size (width, height));
  out.set (cv::VIDEOWRITER_PROP_QUALITY, 100);

  std::uniform_int_distribution<std::int32_t> x (0, width - 8);
  std::uniform_int_distribution<std::int32_t> y (0, height - 8);
  std::uniform_int_distribution<std::int32_t> color (0, 1);
  cv::Mat frame{};
  while (in.read (frame))
    {
      for (std::size_t i = 0; i < squares_per_frame; ++i)
        {
          cv::rectangle (frame, cv::Rect (x (rng), y (rng), 8, 8),
                         cv::Scalar::all (color (rng) * 255), cv::FILLED);
        }
      out.write (frame);
    }
}

void
test_video (std::mt19937 &rng, const std::filesystem::path &dir)
{
  const auto data = random_bytes (rng, 4000);

  for (const auto block_size : BLOCK_SIZES)
    {
      const auto path = dir / std::format ("block{}.avi", block_size);
      const auto ec = write_video (path, data, { 160, 120, 30, block_size });
      check (!ec, std::format ("write block size {}: {}", block_size,
                               ec.message ()));
      const auto read_back = read_video (path);
      check (read_back && *read_back == data,
             std::format ("video round trip with block size {}", block_size));
    }

  // dimensions that are not a multiple of the block size
  const auto odd_path = dir / "odd.avi";
  check (!write_video (odd_path, data, { 163, 101, 30, 8 }),
         "write odd dimensions");
  const auto odd = read_video (odd_path);
  check (odd && *odd == data, "odd dimensions round trip");

  const auto damaged_path = dir / "damaged.avi";
  damage_video (rng, dir / "block4.avi", damaged_path, 1);
  const auto repaired = read_video (damaged_path);
  check (repaired && *repaired == data, "damaged video is repaired");

  const auto noise_path = dir / "noise.avi";
  {
    cv::VideoWriter noise (noise_path.string (),
                           cv::VideoWriter::fourcc ('M', 'J', 'P', 'G'), 30,
                           cv::Size (160, 120));
    cv::Mat frame (120, 160, CV_8UC3);
    for (int i = 0; i < 20; ++i)
      {
        cv::randu (frame, 0, 256);
        noise.write (frame);
      }
  }
  check (!read_video (noise_path), "noise video is rejected");

  const auto garbage_path = dir / "garbage.avi";
  {
    std::ofstream garbage (garbage_path, std::ios::binary);
    garbage << "definitely not a video";
  }
  check (!read_video (garbage_path), "non-video file is rejected");
  check (!read_video (dir / "missing.avi"), "missing file is rejected");
}

void
test_end_to_end (const std::filesystem::path &dir)
{
  const auto source = dir / "secret.txt";
  {
    std::ofstream out (source, std::ios::binary);
    out << "the quick brown fox";
  }

  const file input{ source };
  const auto encrypted = encrypt (pack_file (input), "hunter2");
  const auto serialized = serialize_encrypted_data (*encrypted);
  const auto video_path = dir / "secret.avi";
  check (!write_video (video_path, *serialized, { 320, 240, 30, 4 }),
         "end to end write");

  const auto data = read_video (video_path);
  const auto deserialized = deserialize_encrypted_data (*data);
  check (!decrypt (*deserialized, "hunter3"),
         "end to end wrong key is rejected");
  const auto decrypted = decrypt (*deserialized, "hunter2");
  const auto restored = unpack_file (*decrypted);
  check (restored && restored->path () == "secret.txt"
             && restored->data () == input.data (),
         "end to end round trip");
}

} // namespace

int
main ()
{
  std::mt19937 rng{ 1234 };

  const auto dir = std::filesystem::temp_directory_path ()
                   / std::format ("ftv_tests_{}", std::random_device{}());
  std::filesystem::create_directories (dir);

  test_reed_solomon (rng);
  test_header (rng);
  test_serialize (rng);
  test_payload (rng);
  test_crypto (rng);
  test_cli ();
  test_video (rng, dir);
  test_end_to_end (dir);

  std::filesystem::remove_all (dir);

  if (failures == 0)
    {
      std::println ("all tests passed");
    }
  return failures;
}
