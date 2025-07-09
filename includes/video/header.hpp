#pragma once

#include "ecc/reed_solomon.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace ftv
{

// side lengths in pixels of the square that stores one bit
inline constexpr std::array<std::size_t, 4> BLOCK_SIZES{ 1, 2, 4, 8 };

// guards against absurd sizes read from a damaged or hostile video
inline constexpr std::uint64_t MAX_PAYLOAD_SIZE{ 1ULL << 40 };

// header layout, sizes are little-endian:
// [magic "FTV" (3 bytes)][format version (1 byte)][block size (1 byte)]
// [reserved (3 bytes)][payload size (8 bytes)]
// followed by reed-solomon parity, since everything depends on it
inline constexpr std::size_t HEADER_SIZE{ 16 };
inline constexpr std::size_t ENCODED_HEADER_SIZE{ HEADER_SIZE
                                                  + RS_PARITY_SIZE };

struct header
{
  std::size_t block_size{};
  std::uint64_t payload_size{};
};

[[nodiscard]] std::vector<std::byte> encode_header (const header &h);

// fails with std::errc::not_supported for a newer format version and
// std::errc::bad_message for anything that is not a valid header
[[nodiscard]] std::expected<header, std::error_code>
decode_header (std::span<const std::byte> encoded) noexcept;

} // namespace ftv
