#include <algorithm>
#include <video/pixel.hpp>

namespace ftv
{

std::vector<pixel>
bytes_to_pixels (std::span<const std::byte> bytes) noexcept
{
  // each byte is represented by 2 pixels. each pixel holding
  // a single bit in its 4 colour channels (rgba)
  std::vector<pixel> pixels (bytes.size () * 2);

  for (size_t i = 0; const std::byte &byte : bytes)
    {
      // first pixel gets high nibble
      pixels.at (i * 2)
          = { static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x80 }) ? 255 : 0),
              static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x40 }) ? 255 : 0),
              static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x20 }) ? 255 : 0),
              static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x10 }) ? 255 : 0) };

      // second pixel gets low nibble
      pixels.at (i * 2 + 1)
          = { static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x08 }) ? 255 : 0),
              static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x04 }) ? 255 : 0),
              static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x02 }) ? 255 : 0),
              static_cast<uint8_t> (
                  std::to_integer<int> (byte & std::byte{ 0x01 }) ? 255 : 0) };

      ++i;
    }

  return pixels;
}

std::vector<std::byte>
pixels_to_bytes (std::span<const pixel> pixels) noexcept
{
  std::vector<std::byte> bytes (pixels.size () / 2);

  for (std::size_t i = 0; i < bytes.size (); ++i)
    {
      auto &byte = bytes.at (i);
      const auto &first = pixels[i * 2];
      const auto &second = pixels[i * 2 + 1];

      byte = std::byte{ static_cast<unsigned char> (
          (first.r << 7) | (first.g << 6) | (first.b << 5) | (first.a << 4)
          | (second.r << 3) | (second.g << 2) | (second.b << 1)
          | (second.a)) };
    }

  return bytes;
}

} // namespace ftv
