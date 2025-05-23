#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace ftv
{

// reed-solomon over GF(2^8). a codeword holds up to RS_DATA_SIZE bytes of
// data plus RS_PARITY_SIZE parity bytes and survives up to
// RS_PARITY_SIZE / 2 corrupted bytes
inline constexpr std::size_t RS_CODEWORD_SIZE{ 255 };
inline constexpr std::size_t RS_PARITY_SIZE{ 32 };
inline constexpr std::size_t RS_DATA_SIZE{ RS_CODEWORD_SIZE - RS_PARITY_SIZE };

// returns message followed by its parity bytes. message must not be longer
// than RS_DATA_SIZE
[[nodiscard]] std::vector<std::byte>
rs_encode_block (std::span<const std::byte> message);

// corrects a codeword produced by rs_encode_block and returns the message
[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
rs_decode_block (std::span<const std::byte> codeword) noexcept;

// splits data into full codewords and interleaves them byte by byte, so a
// burst of damaged bytes (a region of a frame, or a whole frame) is spread
// thinly across all codewords instead of overwhelming one
[[nodiscard]] std::size_t rs_encoded_size (std::size_t data_size) noexcept;

[[nodiscard]] std::vector<std::byte>
rs_encode (std::span<const std::byte> data);

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
rs_decode (std::span<const std::byte> encoded, std::size_t data_size) noexcept;

} // namespace ftv
