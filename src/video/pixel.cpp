#include <video/pixel.hpp>

#include <cstdlib>

namespace ftv
{

[[nodiscard]] std::vector<pixel>
bytes_to_pixels (std::span<const std::byte> bytes) noexcept
{
  std::vector<pixel> pixels;
  pixels.reserve (bytes.size () * 8); // Each byte becomes 8 pixels
  for (const auto &byte : bytes)
    {
      const auto value = std::to_integer<std::uint8_t> (byte);
      // Convert each bit to a pixel
      for (int bit = 7; bit >= 0; --bit) // MSB first
        {
          uint8_t pixel_val
              = ((value >> bit) & 1) ? 255 : 0; // White for 1, black for 0
          pixels.push_back ({
              pixel_val, // R
              pixel_val, // G
              pixel_val  // B
          });
        }
    }
  return pixels;
}

[[nodiscard]] std::vector<std::byte>
pixels_to_bytes (std::span<const pixel> pixels) noexcept
{
  std::vector<std::byte> bytes;
  bytes.reserve ((pixels.size () + 7) / 8); // Round up division

  for (size_t i = 0; i < pixels.size (); i += 8)
    {
      uint8_t byte_val = 0;
      // Reconstruct byte from 8 pixels
      for (size_t bit = 0; bit < 8 && (i + bit) < pixels.size (); ++bit)
        {
          // Consider pixel white (1) if any channel is closer to 255 than to 0
          if ((pixels[i + bit].r > 127) || (pixels[i + bit].g > 127)
              || (pixels[i + bit].b > 127))
            {
              byte_val |= (1 << (7 - bit)); // MSB first
            }
        }
      bytes.push_back (std::byte (byte_val));
    }
  return bytes;
}

} // namespace ftv
