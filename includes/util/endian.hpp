#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <span>
#include <vector>

namespace ftv
{

// integers are always stored little-endian so data written on one platform
// can be read on another

template <std::unsigned_integral T>
void
put_le (std::vector<std::byte> &out, T value)
{
  if constexpr (std::endian::native == std::endian::big)
    {
      value = std::byteswap (value);
    }
  const auto bytes = std::bit_cast<std::array<std::byte, sizeof (T)>> (value);
  out.insert (out.end (), bytes.begin (), bytes.end ());
}

// caller guarantees that in holds at least offset + sizeof (T) bytes
template <std::unsigned_integral T>
[[nodiscard]] T
get_le (std::span<const std::byte> in, std::size_t offset)
{
  std::array<std::byte, sizeof (T)> bytes{};
  std::ranges::copy (in.subspan (offset, sizeof (T)), bytes.begin ());
  auto value = std::bit_cast<T> (bytes);
  if constexpr (std::endian::native == std::endian::big)
    {
      value = std::byteswap (value);
    }
  return value;
}

} // namespace ftv
