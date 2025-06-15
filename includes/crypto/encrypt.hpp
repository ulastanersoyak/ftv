#pragma once

#include "crypto/encrypted_data.hpp"
#include "crypto/secure_key.hpp"

#include <cstddef>
#include <expected>
#include <functional>
#include <span>
#include <string_view>
#include <system_error>

namespace ftv
{

inline constexpr std::size_t AES_GCM_IV_SIZE{ 12 };
inline constexpr std::size_t AES_GCM_TAG_SIZE{ 16 };

using encryption_func
    = std::function<std::expected<encrypted_data, std::error_code> (
        std::span<const std::byte> data, const secure_key &key)>;

[[nodiscard]] std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, const secure_key &key) noexcept;

// derives a key from password with a fresh random salt and encrypts data
// with it. the salt is stored in the result
[[nodiscard]] std::expected<encrypted_data, std::error_code>
encrypt (std::span<const std::byte> data, std::string_view password,
         encryption_func fn = aes_256_gcm) noexcept;

} // namespace ftv
