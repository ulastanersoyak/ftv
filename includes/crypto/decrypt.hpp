#pragma once

#include "crypto/encrypt.hpp"
#include "crypto/encrypted_data.hpp"
#include "file/file.hpp"

namespace ftv
{

using decryption_func = std::function<std::expected<file, std::error_code> (
    const encrypted_data &data, const secure_key &key)>;

std::expected<file, std::error_code>
aes_256_gcm_decrypt (const encrypted_data &encrypted, const secure_key &key);

[[nodiscard]] std::expected<file, std::error_code>
decrypt (const encrypted_data &data, const secure_key &key,
         decryption_func fn = aes_256_gcm_decrypt) noexcept;

} // namespace ftv
