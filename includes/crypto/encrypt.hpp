#pragma once

#include "crypto/secure_key.hpp"
#include "file/file.hpp"

#include <expected>
#include <functional>

namespace ftv
{

struct encrypted_data
{
  std::vector<std::byte> ciphertext;
  std::vector<std::byte> init_vec; // generated during encryption
  std::vector<std::byte> tag;      // generated during encryption
};

using encryption_func
    = std::function<std::expected<encrypted_data, std::error_code> (
        std::span<const std::byte> data, const secure_key &key)>;

std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, const secure_key &key);

[[nodiscard]] std::expected<encrypted_data, std::error_code>
encrypt (const file &source, const secure_key &key,
         encryption_func fn = aes_256_gcm);

} // namespace ftv
