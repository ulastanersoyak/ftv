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
  metadata (std::string fname, std::size_t fsize, std::size_t checksum,
            std::size_t fps, const resolution &res);

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
  std::size_t filename_size_{};
  std::string filename_{};
  std::size_t file_size_{};
  std::size_t checksum_{};
  std::size_t fps_{};
  resolution res_{};
};

} // namespace ftv
