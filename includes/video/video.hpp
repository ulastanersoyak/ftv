#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>
#include <vector>

namespace ftv
{

struct video_options
{
  std::size_t width{};
  std::size_t height{};
  std::size_t fps{};
  std::size_t block_size{};
};

// writes data as a black and white video where every
// block_size x block_size square of pixels holds one bit. the bits start
// with a header and the data is protected with reed-solomon codes, so a
// moderate amount of compression damage is repaired when reading
[[nodiscard]] std::error_code
write_video (const std::filesystem::path &path,
             std::span<const std::byte> data,
             const video_options &options) noexcept;

// reads data written by write_video. the block size is detected from the
// header
[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
read_video (const std::filesystem::path &path) noexcept;

} // namespace ftv
