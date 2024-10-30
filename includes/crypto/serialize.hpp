#include "crypto/encrypted_data.hpp"

#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace ftv
{

// serialized encrypted data:
// [iv size (4 bytes)][iv data][tag size (4 bytes)][tag data][ciphertext]

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
serialize_encrypted_data (const encrypted_data &data) noexcept;

[[nodiscard]]
std::expected<encrypted_data, std::error_code> deserialize_encrypted_data (
    std::span<const std::byte> serialized_data) noexcept;

} // namespace ftv
