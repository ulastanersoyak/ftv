#include "crypto/checksum.hpp"
#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "video/metadata.hpp"
#include "video/opencv/video.hpp"
#include "video/pixel.hpp"

#include <filesystem>
#include <print>
#include <string>
#include <vector>

struct parameters
{
  std::vector<std::string> input_files{};
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
  std::println ("encrypt: ftv encrypt <input_files...> -o <output_file> -k "
                "<key> [options]");
  std::println ("decrypt: ftv decrypt <input_file> -k <key>");
  std::println ("\noptions:");
  std::println ("  -o, --output <file>    output video file path (required "
                "for encrypt only)");
  std::println (
      "  -k, --key <key>        encryption/decryption key (min 8 chars)");
  std::println (
      "  -w, --width <pixels>   video width (100-4096, default: 300)");
  std::println (
      "  -h, --height <pixels>  video height (100-4096, default: 300)");
  std::println (
      "  -f, --fps <number>     frames per second (1-60, default: 30)");
  std::println ("  --help                 show this help message");
  std::println ("\nexample:");
  std::println ("  ftv encrypt file1.txt file2.txt -o video.avi -k mypassword "
                "-w 640 -h 480 -f 30");
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
      return "input file path is required";
    case validation_error::missing_output:
      return "output file path is required for encryption";
    case validation_error::key_too_long:
      return "encryption key must be smaller than 32 characters";
    case validation_error::input_not_found:
      return "one or more input files do not exist";
    case validation_error::invalid_width:
      return "width must be between 100 and 4096";
    case validation_error::invalid_height:
      return "height must be between 100 and 4096";
    case validation_error::invalid_fps:
      return "fps must be between 1 and 60";
    default:
      return "unknown validation error";
    }
}

validation_error
validate_parameters (const parameters &params)
{
  if (params.input_files.empty ())
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

  for (const auto &file : params.input_files)
    {
      if (!std::filesystem::exists (file))
        {
          return validation_error::input_not_found;
        }
    }

  if (params.encrypt)
    {
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
    }

  return validation_error::success;
}

bool
process_single_encrypt (const std::string &input_file,
                        const std::string &output_file, const std::string &key,
                        std::size_t width, std::size_t height, std::size_t fps)
{
  try
    {
      std::println ("encrypting file: {}", input_file);

      ftv::file f{ input_file };
      ftv::secure_key secure_key{ key };

      auto encrypted = ftv::encrypt (f, secure_key, ftv::aes_256_gcm);
      if (!encrypted)
        {
          std::println (stderr, "error: encryption failed");
          return false;
        }

      auto serialized = ftv::serialize_encrypted_data (*encrypted);
      if (!serialized)
        {
          std::println (stderr, "error: serialization failed");
          return false;
        }

      ftv::metadata data{
        input_file, f.size (), ftv::hash (*serialized), fps, { width, height }
      };

      ftv::video vid{ output_file, data };
      auto ec = vid.write (*serialized);
      if (ec.value () != 0)
        {
          std::println (stderr, "error: video writing failed with code: {}",
                        ec.value ());
          return false;
        }

      // Delete source file after successful encryption
      std::filesystem::remove (input_file);

      std::println ("successfully encrypted to: {}", output_file);
      return true;
    }
  catch (const std::exception &e)
    {
      std::println (stderr, "error: {}", e.what ());
      return false;
    }
}

bool
process_encrypt (const parameters &params)
{
  bool all_success = true;
  for (const auto &input_file : params.input_files)
    {
      // Generate output filename for each input file
      std::filesystem::path output_path = params.output_file;
      if (params.input_files.size () > 1)
        {
          auto stem = std::filesystem::path (input_file).stem ().string ();
          output_path = output_path.parent_path ()
                        / (stem + "_" + output_path.filename ().string ());
        }

      if (!process_single_encrypt (input_file, output_path.string (),
                                   params.key, params.width, params.height,
                                   params.fps))
        {
          all_success = false;
        }
    }
  return all_success;
}

bool
process_decrypt (const parameters &params)
{
  try
    {
      std::println ("decrypting video: {}", params.input_files[0]);

      ftv::video vid{ params.input_files[0] };
      auto pixels = vid.read ();
      if (!pixels)
        {
          std::println (stderr, "error: failed to read video");
          return false;
        }

      auto bytes_from_vid = ftv::pixels_to_bytes (*pixels);
      auto deserialized = ftv::deserialize_encrypted_data (bytes_from_vid);
      if (!deserialized)
        {
          std::println (stderr, "error: deserialization failed");
          return false;
        }

      const auto metadata = vid.get_metadata ();
      const auto hash = metadata.checksum ();
      if (hash != metadata.checksum ())
        {
          std::println (stderr, "error: data corruption detected");
          return false;
        }

      ftv::secure_key key{ params.key };
      auto file = ftv::decrypt (*deserialized, key);
      if (!file)
        {
          std::println (stderr, "error: decryption failed");
          return false;
        }

      const auto &original_path = metadata.filename ();

      auto write_result = ftv::write (*file, original_path);
      if (!write_result)
        {
          std::println (stderr, "error: failed to write file: {}",
                        original_path);
          return false;
        }

      // Delete video file after successful decryption
      std::filesystem::remove (params.input_files[0]);

      std::println ("successfully decrypted to: {}", original_path);
      return true;
    }
  catch (const std::exception &e)
    {
      std::println (stderr, "error: {}", e.what ());
      return false;
    }
}

std::optional<parameters>
parse_args (int argc, char *argv[])
{
  if (argc < 2)
    {
      print_usage ();
      return std::nullopt;
    }

  if (std::string (argv[1]) == "--help")
    {
      print_usage ();
      return std::nullopt;
    }

  parameters params;

  std::string operation = argv[1];
  if (operation == "encrypt")
    {
      params.encrypt = true;
    }
  else if (operation == "decrypt")
    {
      params.encrypt = false;
    }
  else
    {
      std::println (
          stderr,
          "error: first argument must be either 'encrypt' or 'decrypt'");
      return std::nullopt;
    }

  int i = 2;
  // Collect input files until we hit an option or end of args
  while (i < argc && argv[i][0] != '-')
    {
      params.input_files.push_back (argv[i++]);
    }

  // Parse remaining options
  for (; i < argc; i++)
    {
      std::string_view arg = argv[i];

      if ((arg == "-o" || arg == "--output") && i + 1 < argc)
        {
          params.output_file = argv[++i];
        }
      else if ((arg == "-k" || arg == "--key") && i + 1 < argc)
        {
          params.key = argv[++i];
        }
      else if ((arg == "-w" || arg == "--width") && i + 1 < argc)
        {
          try
            {
              params.width = std::stoull (argv[++i]);
            }
          catch (const std::exception &e)
            {
              std::println (stderr, "error: invalid width value");
              return std::nullopt;
            }
        }
      else if ((arg == "-h" || arg == "--height") && i + 1 < argc)
        {
          try
            {
              params.height = std::stoull (argv[++i]);
            }
          catch (const std::exception &e)
            {
              std::println (stderr, "error: invalid height value");
              return std::nullopt;
            }
        }
      else if ((arg == "-f" || arg == "--fps") && i + 1 < argc)
        {
          try
            {
              params.fps = std::stoull (argv[++i]);
            }
          catch (const std::exception &e)
            {
              std::println (stderr, "error: invalid fps value");
              return std::nullopt;
            }
        }
      else
        {
          std::println (stderr, "error: unknown argument: {}", arg);
          return std::nullopt;
        }
    }

  return params;
}

int
main (int argc, char *argv[])
{
  try
    {
      auto params = parse_args (argc, argv);
      if (!params)
        {
          return 1;
        }

      auto validation_result = validate_parameters (*params);
      if (validation_result != validation_error::success)
        {
          std::println (stderr, "error: {}",
                        validation_error_to_string (validation_result));
          return static_cast<int> (validation_result);
        }

      bool success = params->encrypt ? process_encrypt (*params)
                                     : process_decrypt (*params);
      return success ? 0 : 1;
    }
  catch (const std::exception &e)
    {
      std::println (stderr, "fatal error: {}", e.what ());
      return 1;
    }
}
