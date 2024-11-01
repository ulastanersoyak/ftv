#include <video/pixel.hpp>

namespace ftv
{
std::vector<pixel>
bytes_to_pixels (std::span<const std::byte> bytes) noexcept
{
  std::vector<pixel> pixels (bytes.size () * 2);
  for (size_t i = 0; i < bytes.size (); ++i)
    {
      const std::byte &byte = bytes[i];

      // first pixel from high nibble
      pixels.at (i * 2) = {
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x80 }) != 0 ? 255
                                                                     : 0),
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x40 }) != 0 ? 255
                                                                     : 0),
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x20 }) != 0 ? 255
                                                                     : 0),
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x10 }) != 0 ? 255 : 0)
      };

      // second pixel from low nibble
      pixels.at (i * 2 + 1) = {
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x08 }) != 0 ? 255
                                                                     : 0),
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x04 }) != 0 ? 255
                                                                     : 0),
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x02 }) != 0 ? 255
                                                                     : 0),
        static_cast<uint8_t> (
            std::to_integer<uint8_t> (byte & std::byte{ 0x01 }) != 0 ? 255 : 0)
      };
    }
  return pixels;
}

std::vector<std::byte>
pixels_to_bytes (std::span<const pixel> pixels) noexcept
{
  std::vector<std::byte> bytes (pixels.size () / 2);

  for (std::size_t i = 0; i < bytes.size (); ++i)
    {
      const auto &first = pixels[i * 2];
      const auto &second = pixels[i * 2 + 1];

      uint8_t byte_value = 0;

      // high nibble from first pixel
      byte_value |= (first.r != 0 ? 0x80 : 0);
      byte_value |= (first.g != 0 ? 0x40 : 0);
      byte_value |= (first.b != 0 ? 0x20 : 0);
      byte_value |= (first.a != 0 ? 0x10 : 0);

      // low nibble from second pixel
      byte_value |= (second.r != 0 ? 0x08 : 0);
      byte_value |= (second.g != 0 ? 0x04 : 0);
      byte_value |= (second.b != 0 ? 0x02 : 0);
      byte_value |= (second.a != 0 ? 0x01 : 0);

      bytes.at (i) = std::byte{ byte_value };
    }

  return bytes;
}
} // namespace ftv
