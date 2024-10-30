#include <expected>
#include <vector>

#include "crypto/encrypt.hpp"

namespace ftv
{

// serialized encryopted data:
// [iv size (4 bytes)][iv data][tag size (4 bytes)][tag data][ciphertext]

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
serialize_encrypted_data (const encrypted_data &data);

[[nodiscard]]
std::expected<encrypted_data, std::error_code>
deserialize_encrypted_data (std::span<const std::byte> serialized_data);

} // namespace ftv
