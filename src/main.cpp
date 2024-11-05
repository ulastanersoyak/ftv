#include "crypto/checksum.hpp"
#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "video/metadata.hpp"
#include "video/pixel.hpp"
#include "video/video.hpp"

#include <filesystem>
#include <print>
#include <string_view>
#include <vector>

struct parameters
{
  std::string input_file{};
  std::string output_file{};
  std::string key{};
  std::size_t width = 300;
  std::size_t height = 300;
  std::size_t fps = 30;
  bool encrypt = false; // false = decrypt
};

void
print_usage ()
{
  std::println ("usage:");
  std::println ("encrypt: ftv encrypt <input_file> -o <output_file> -k "
                "<key> [options]");
  std::println ("decrypt: ftv decrypt <input_file> -k <key>");
  std::println ("\noptions:");
  std::println ("  -o, --output <file>    output video file path (required "
                "for encrypt only)");
  std::println (
      "  -k, --key <key>        encryption/decryption key (max 32 chars)");
  std::println (
      "  -w, --width <pixels>   video width (100-4096, default: 300)");
  std::println (
      "  -h, --height <pixels>  video height (100-4096, default: 300)");
  std::println (
      "  -f, --fps <number>     frames per second (1-60, default: 30)");
  std::println ("  -h, --help                 show this help message");
  std::println ("\nexample:");
  std::println (
      "  ftv encrypt file.txt -o video.avi -k mypassword -w 640 -h 480 -f 30");
  std::println ("  ftv decrypt video.avi -k mypassword");
}

enum class validation_error
{
  success = 0,
  missing_input = 1,
  missing_output = 2,
  key_too_long = 3,
  input_not_found = 4,
  invalid_width = 5,
  invalid_height = 6,
  invalid_fps = 7
};

std::string
validation_error_to_string (validation_error error)
{
  switch (error)
    {
    case validation_error::missing_input:
      {
        return "input file path is required";
      }
    case validation_error::missing_output:
      {
        return "output file path is required for encryption";
      }
    case validation_error::key_too_long:
      {
        return "encryption key must be smaller than 32 characters";
      }
    case validation_error::input_not_found:
      {
        return "input file does not exist";
      }
    case validation_error::invalid_width:
      {
        return "width must be between 100 and 4096";
      }
    case validation_error::invalid_height:
      {
        return "height must be between 100 and 4096";
      }
    case validation_error::invalid_fps:
      {
        return "fps must be between 1 and 60";
      }
    default:
      {
        return "unknown validation error";
      }
    }
}

validation_error
validate_parameters (const parameters &params)
{
  if (params.input_file.empty ())
    {
      return validation_error::missing_input;
    }

  if (params.encrypt && params.output_file.empty ())
    {
      return validation_error::missing_output;
    }

  if (params.key.length () > 32)
    {
      return validation_error::key_too_long;
    }

  if (!std::filesystem::exists (params.input_file))
    {
      return validation_error::input_not_found;
    }

  if (params.width < 100 || params.width > 4096)
    {
      return validation_error::invalid_width;
    }

  if (params.height < 100 || params.height > 4096)
    {
      return validation_error::invalid_height;
    }

  if (params.fps < 1 || params.fps > 60)
    {
      return validation_error::invalid_fps;
    }

  return validation_error::success;
}

parameters
parse_arguments (int argc, char **argv)
{
  parameters params{};

  if (argc == 2)
    {
      std::string_view arg = argv[1];
      if (arg == "-h" || arg == "--help")
        {
          print_usage ();
          std::exit (0);
        }
    }

  if (argc < 2)
    {
      print_usage ();
      std::exit (1);
    }

  std::string_view command = argv[1];
  params.encrypt = (command == "encrypt");

  if (argc > 2)
    {
      params.input_file = argv[2];
    }

  for (int i = 3; i < argc; ++i)
    {
      std::string_view arg = argv[i];

      if (arg == "-h" || arg == "--help")
        {
          print_usage ();
          std::exit (0);
        }

      if (arg == "-o" || arg == "--output")
        {
          if (++i < argc)
            {
              params.output_file = argv[i];
            }
          continue;
        }

      if (arg == "-k" || arg == "--key")
        {
          if (++i < argc)
            {
              params.key = argv[i];
            }
          continue;
        }

      if (arg == "-w" || arg == "--width")
        {
          if (++i < argc)
            {
              params.width = std::stoul (argv[i]);
            }
          continue;
        }

      if (arg == "-h" || arg == "--height")
        {
          if (++i < argc)
            {
              params.height = std::stoul (argv[i]);
            }
          continue;
        }

      if (arg == "-f" || arg == "--fps")
        {
          if (++i < argc)
            {
              params.fps = std::stoul (argv[i]);
            }
          continue;
        }
    }

  return params;
}

int
main (int argc, char **argv)
{
  auto params = parse_arguments (argc, argv);

  auto validation_result = validate_parameters (params);
  if (validation_result != validation_error::success)
    {
      std::println ("error: {}",
                    validation_error_to_string (validation_result));
      return 1;
    }

  using namespace std::string_view_literals;
  ftv::secure_key key{ params.key };

  if (params.encrypt)
    {
      ftv::file f{ params.input_file };
      const auto encrypted = ftv::encrypt (f, key, ftv::aes_256_gcm);
      if (!encrypted)
        {
          std::println ("error encrypting file: {}", params.input_file);
          return 1;
        }

      const auto serialized = ftv::serialize_encrypted_data (*encrypted);
      if (!serialized)
        {
          std::println ("error serializing encrypted data: {}",
                        params.input_file);
          return 1;
        }

      ftv::metadata data{ params.input_file,
                          serialized->size (),
                          ftv::hash (*serialized),
                          params.fps,
                          { params.width, params.height } };

      ftv::video vid{ params.output_file, data };
      auto ec = vid.write (*serialized);
      if (ec.value () != 0)
        {
          std::println ("error writing video file: {}", params.output_file);
          return 1;
        }

      std::println ("successfully encrypted {} to {}", params.input_file,
                    params.output_file);
    }
  else
    {
      ftv::video vid{ params.input_file };
      const auto pixels = vid.read ();
      if (!pixels)
        {
          std::println ("error reading video file: {}", params.input_file);
          return 1;
        }

      const auto bytes_from_vid = ftv::pixels_to_bytes (*pixels);

      if (ftv::hash (bytes_from_vid) != vid.get_metadata ().checksum ())
        {
          std::println ("data corruption on video file: {}",
                        params.input_file);
          return 1;
        }

      const auto deserialized
          = ftv::deserialize_encrypted_data (bytes_from_vid);
      if (!deserialized)
        {
          std::println ("error deserializing video data");
          return 1;
        }

      const auto file = ftv::decrypt (*deserialized, key);
      if (!file)
        {
          std::println ("error decrypting data");
          return 1;
        }

      auto output_path = vid.get_metadata ().filename ().append ("_decrypted");
      auto write_result = ftv::write (*file, output_path);
      if (write_result.value () != 0)
        {
          std::println ("error writing decrypted file: {}", output_path);
          return 1;
        }

      std::println ("successfully decrypted {} to {}", params.input_file,
                    output_path);
    }

  return 0;
}
