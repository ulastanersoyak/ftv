#pragma once

#include <cstdint>
#include <span>
#include <vector>
namespace ftv
{

struct pixel
{
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};
};

[[nodiscard]] std::vector<pixel>
bytes_to_pixels (std::span<const std::byte> bytes) noexcept;

[[nodiscard]] std::vector<std::byte>
pixels_to_bytes (std::span<const pixel> bytes) noexcept;

} // namespace ftv
