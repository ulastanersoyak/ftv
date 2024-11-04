#pragma once

#include "video/resolution.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
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
  metadata (std::string fname, std::size_t fsize, std::size_t checksum,
            std::size_t fps, const resolution &res);
  explicit metadata (const std::filesystem::path &video_path);

  [[nodiscard]] std::size_t filename_size () const noexcept;
  [[nodiscard]] std::string filename () const noexcept;
  [[nodiscard]] std::size_t file_size () const noexcept;
  [[nodiscard]] std::size_t checksum () const noexcept;
  [[nodiscard]] std::size_t fps () const noexcept;
  [[nodiscard]] resolution res () const noexcept;

  [[nodiscard]] constexpr std::size_t
  size () const noexcept
  {
    return sizeof (std::size_t) +    // filename_size (8 bytes)
           this->filename_.size () + // filename
           sizeof (std::size_t) +    // file_size (8 bytes)
           sizeof (std::size_t) +    // checksum (8 bytes)
           sizeof (std::size_t) +    // fps (8 bytes)
           sizeof (resolution);      // resolution(16 bytes)
  }

  [[nodiscard]] std::vector<std::byte> to_vec () const noexcept;

private:
  std::size_t filename_size_{ 0 };
  std::string filename_{ 0 };
  std::size_t file_size_{ 0 };
  std::size_t checksum_{ 0 };
  std::size_t fps_{ 0 };
  resolution res_{ 0 };
};

} // namespace ftv
