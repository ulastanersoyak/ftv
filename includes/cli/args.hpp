#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace ftv
{

enum class command
{
  encrypt,
  decrypt
};

struct parameters
{
  command cmd{ command::encrypt };
  std::string input_file{};
  std::string output_file{}; // optional for decrypt
  std::string key{};
  std::size_t width{ 300 };
  std::size_t height{ 300 };
  std::size_t fps{ 30 };
  bool help{ false };
};

[[nodiscard]] std::string usage ();

// parses the arguments that follow the program name. on failure returns a
// message describing the problem
[[nodiscard]] std::expected<parameters, std::string>
parse_arguments (std::span<const std::string_view> args);

} // namespace ftv
