#pragma once

#include "file/file.hpp"

#include <expected>
#include <functional>
#include <string_view>

namespace ftv
{

struct encrypted_data
{
  std::vector<std::byte> ciphertext;
  std::vector<std::byte> iv;  // generated during encryption
  std::vector<std::byte> tag; // generated during encryption
};

using encryption_func
    = std::function<std::expected<encrypted_data, std::error_code> (
        std::span<const std::byte> data, std::string_view key)>;

std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, std::string_view key);

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
encrypt (const file &source, std::string_view key,
         encryption_func fn = aes_256_gcm);

} // namespace ftv
