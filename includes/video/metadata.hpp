#pragma once

#include "video/resolution.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace ftv
{

class metadata
{
public:
  metadata () = default;

  explicit metadata (std::span<const std::byte>);

  metadata (std::string fname, std::size_t fsize, std::size_t sum,
            std::size_t frames_per_sec, const resolution &r);

  [[nodiscard]] constexpr std::size_t
  size () const noexcept
  {
    return sizeof (std::size_t) + // filename_size (8 bytes)
           filename.size () +     // filename
           sizeof (std::size_t) + // file_size (8 bytes)
           sizeof (std::size_t) + // checksum (8 bytes)
           sizeof (std::size_t) + // fps (8 bytes)
           sizeof (resolution);   // resolution(16 bytes)
  }

  [[nodiscard]] std::vector<std::byte> to_vec () const noexcept;

private:
  std::size_t filename_size{};
  std::string filename{};
  std::size_t file_size{};
  std::size_t checksum{};
  std::size_t fps{};
  resolution res{};
};

} // namespace ftv
