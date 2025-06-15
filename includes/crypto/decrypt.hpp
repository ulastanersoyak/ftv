#pragma once

#include "crypto/encrypted_data.hpp"
#include "crypto/secure_key.hpp"

#include <cstddef>
#include <expected>
#include <functional>
#include <string_view>
#include <system_error>
#include <vector>

namespace ftv
{

using decryption_func
    = std::function<std::expected<std::vector<std::byte>, std::error_code> (
        const encrypted_data &data, const secure_key &key)>;

// fails with std::errc::bad_message when the key is wrong or the data was
// modified
[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
aes_256_gcm_decrypt (const encrypted_data &encrypted,
                     const secure_key &key) noexcept;

// derives the key from password and the salt stored in data, then decrypts
[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
decrypt (const encrypted_data &data, std::string_view password,
         decryption_func fn = aes_256_gcm_decrypt) noexcept;

} // namespace ftv
