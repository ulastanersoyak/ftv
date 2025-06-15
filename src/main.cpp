#include "cli/args.hpp"
#include "crypto/checksum.hpp"
#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"
#include "video/metadata.hpp"
#include "video/pixel.hpp"
#include "video/video.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <print>
#include <string_view>
#include <vector>

namespace
{

int
run_encrypt (const ftv::parameters &params)
{
  const ftv::file input{ params.input_file };

  const auto encrypted = ftv::encrypt (input.data (), params.key);
  if (!encrypted)
    {
      std::println (stderr, "error encrypting {}: {}", params.input_file,
                    encrypted.error ().message ());
      return 1;
    }

  const auto serialized = ftv::serialize_encrypted_data (*encrypted);
  if (!serialized)
    {
      std::println (stderr, "error serializing encrypted data: {}",
                    serialized.error ().message ());
      return 1;
    }

  const ftv::metadata data{ params.input_file,
                            serialized->size (),
                            ftv::hash (*serialized),
                            params.fps,
                            { params.width, params.height } };

  const ftv::video vid{ params.output_file, data };
  if (const auto ec = vid.write (*serialized))
    {
      std::println (stderr, "error writing video file {}: {}",
                    params.output_file, ec.message ());
      return 1;
    }

  std::println ("successfully encrypted {} to {}", params.input_file,
                params.output_file);
  return 0;
}

int
run_decrypt (const ftv::parameters &params)
{
  ftv::video vid{ params.input_file };

  const auto pixels = vid.read ();
  if (!pixels)
    {
      std::println (stderr, "error reading video file {}: {}",
                    params.input_file, pixels.error ().message ());
      return 1;
    }

  const auto bytes_from_vid = ftv::pixels_to_bytes (*pixels);
  if (ftv::hash (bytes_from_vid) != vid.get_metadata ().checksum ())
    {
      std::println (stderr, "data corruption on video file: {}",
                    params.input_file);
      return 1;
    }

  const auto deserialized = ftv::deserialize_encrypted_data (bytes_from_vid);
  if (!deserialized)
    {
      std::println (stderr, "error deserializing video data: {}",
                    deserialized.error ().message ());
      return 1;
    }

  const auto decrypted = ftv::decrypt (*deserialized, params.key);
  if (!decrypted)
    {
      std::println (stderr, "error decrypting data: wrong key or corrupted "
                            "video");
      return 1;
    }

  const std::filesystem::path output_path
      = params.output_file.empty ()
            ? std::filesystem::path{ vid.get_metadata ().filename ()
                                     + "_decrypted" }
            : std::filesystem::path{ params.output_file };

  if (const auto ec = ftv::write (*decrypted, output_path))
    {
      if (ec == std::errc::file_exists)
        {
          std::println (stderr,
                        "error: {} already exists, choose another path with "
                        "-o",
                        output_path.string ());
        }
      else
        {
          std::println (stderr, "error writing decrypted file {}: {}",
                        output_path.string (), ec.message ());
        }
      return 1;
    }

  std::println ("successfully decrypted {} to {}", params.input_file,
                output_path.string ());
  return 0;
}

} // namespace

int
main (int argc, char **argv)
{
  const std::vector<std::string_view> args (argv + 1, argv + argc);

  const auto params = ftv::parse_arguments (args);
  if (!params)
    {
      std::println (stderr, "error: {}\n", params.error ());
      std::print (stderr, "{}", ftv::usage ());
      return 1;
    }

  if (params->help)
    {
      std::print ("{}", ftv::usage ());
      return 0;
    }

  if (!std::filesystem::exists (params->input_file))
    {
      std::println (stderr, "error: input file does not exist: {}",
                    params->input_file);
      return 1;
    }

  try
    {
      return params->cmd == ftv::command::encrypt ? run_encrypt (*params)
                                                  : run_decrypt (*params);
    }
  catch (const std::exception &e)
    {
      std::println (stderr, "error: {}", e.what ());
      return 1;
    }
}
