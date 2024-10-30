#pragma once

#include "crypto/encrypted_data.hpp"
#include "crypto/secure_key.hpp"
#include "file/file.hpp"

#include <expected>
#include <functional>

namespace ftv
{

using encryption_func
    = std::function<std::expected<encrypted_data, std::error_code> (
        std::span<const std::byte> data, const secure_key &key)>;

std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, const secure_key &key) noexcept;

[[nodiscard]] std::expected<encrypted_data, std::error_code>
encrypt (const file &source, const secure_key &key,
         encryption_func fn = aes_256_gcm) noexcept;

} // namespace ftv
