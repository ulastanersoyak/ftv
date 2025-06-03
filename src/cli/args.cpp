#include "cli/args.hpp"
#include "video/resolution.hpp"

#include <charconv>
#include <format>
#include <optional>

namespace ftv
{

namespace
{

[[nodiscard]] std::optional<std::size_t>
parse_number (std::string_view text)
{
  std::size_t value = 0;
  const auto *const end = text.data () + text.size ();
  const auto [ptr, ec] = std::from_chars (text.data (), end, value);
  if (ec != std::errc{} || ptr != end)
    {
      return std::nullopt;
    }
  return value;
}

[[nodiscard]] bool
is_help (std::string_view arg)
{
  return arg == "-h" || arg == "--help";
}

[[nodiscard]] std::optional<std::string>
validate (const parameters &params)
{
  if (params.input_file.empty ())
    {
      return "input file path is required";
    }
  if (params.cmd == command::encrypt && params.output_file.empty ())
    {
      return "output video path (-o) is required for encrypt";
    }
  if (params.key.empty ())
    {
      return "key (-k) is required";
    }
  if (params.key.size () > 32)
    {
      return "key must be at most 32 characters";
    }
  if (params.width < MIN_DIMENSION || params.width > MAX_DIMENSION)
    {
      return std::format ("width must be between {} and {}", MIN_DIMENSION,
                          MAX_DIMENSION);
    }
  if (params.height < MIN_DIMENSION || params.height > MAX_DIMENSION)
    {
      return std::format ("height must be between {} and {}", MIN_DIMENSION,
                          MAX_DIMENSION);
    }
  if (params.fps < 1 || params.fps > MAX_FPS)
    {
      return std::format ("fps must be between 1 and {}", MAX_FPS);
    }
  return std::nullopt;
}

} // namespace

[[nodiscard]] std::string
usage ()
{
  const parameters defaults{};
  return std::format (
      "usage:\n"
      "  ftv encrypt <input_file> -o <output_video> -k <key> [options]\n"
      "  ftv decrypt <input_video> -k <key> [-o <output_file>]\n"
      "\n"
      "options:\n"
      "  -o, --output <path>    where to write the result (required for "
      "encrypt,\n"
      "                         decrypt defaults to <name>_decrypted)\n"
      "  -k, --key <key>        encryption/decryption key (max 32 chars)\n"
      "  -w, --width <pixels>   video width ({0}-{1}, default: {2})\n"
      "  -H, --height <pixels>  video height ({0}-{1}, default: {3})\n"
      "  -f, --fps <number>     frames per second (1-{4}, default: {5})\n"
      "  -h, --help             show this help message\n"
      "\n"
      "example:\n"
      "  ftv encrypt file.txt -o video.avi -k mypassword -w 640 -H 480 -f 30\n"
      "  ftv decrypt video.avi -k mypassword\n",
      MIN_DIMENSION, MAX_DIMENSION, defaults.width, defaults.height, MAX_FPS,
      defaults.fps);
}

[[nodiscard]] std::expected<parameters, std::string>
parse_arguments (std::span<const std::string_view> args)
{
  parameters params{};

  if (args.empty ())
    {
      return std::unexpected ("missing command");
    }
  if (is_help (args[0]))
    {
      params.help = true;
      return params;
    }
  if (args[0] == "encrypt")
    {
      params.cmd = command::encrypt;
    }
  else if (args[0] == "decrypt")
    {
      params.cmd = command::decrypt;
    }
  else
    {
      return std::unexpected (std::format (
          "unknown command '{}', expected 'encrypt' or 'decrypt'", args[0]));
    }

  for (std::size_t i = 1; i < args.size (); ++i)
    {
      const auto arg = args[i];

      if (is_help (arg))
        {
          params.help = true;
          return params;
        }

      if (!arg.starts_with ('-'))
        {
          if (!params.input_file.empty ())
            {
              return std::unexpected (
                  std::format ("unexpected argument '{}'", arg));
            }
          params.input_file = arg;
          continue;
        }

      std::string *text = nullptr;
      std::size_t *number = nullptr;
      if (arg == "-o" || arg == "--output")
        {
          text = &params.output_file;
        }
      else if (arg == "-k" || arg == "--key")
        {
          text = &params.key;
        }
      else if (arg == "-w" || arg == "--width")
        {
          number = &params.width;
        }
      else if (arg == "-H" || arg == "--height")
        {
          number = &params.height;
        }
      else if (arg == "-f" || arg == "--fps")
        {
          number = &params.fps;
        }
      else
        {
          return std::unexpected (std::format ("unknown option '{}'", arg));
        }

      // every option takes a value, which may itself start with '-'
      if (i + 1 >= args.size ())
        {
          return std::unexpected (std::format ("missing value for {}", arg));
        }
      const auto value = args[++i];

      if (text)
        {
          *text = value;
          continue;
        }
      const auto parsed = parse_number (value);
      if (!parsed)
        {
          return std::unexpected (
              std::format ("invalid number for {}: '{}'", arg, value));
        }
      *number = *parsed;
    }

  if (const auto error = validate (params))
    {
      return std::unexpected (*error);
    }
  return params;
}

} // namespace ftv
