#include "crypto/encrypt.hpp"
#include "crypto/evp_cipher_raii.hpp"

#include <expected>
#include <utility>

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace ftv
{

std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, const secure_key &key) noexcept
{
  if (key.size () != 32)
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::invalid_argument)) };
    }

  // generate random iv (12 bytes for GCM)
  std::vector<std::byte> init_vec (12);
  if (RAND_bytes (reinterpret_cast<unsigned char *> (init_vec.data ()),
                  static_cast<std::int32_t> (init_vec.size ()))
      != 1)
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  evp_cipher ctx{};
  std::vector<std::byte> ciphertext (data.size () + EVP_MAX_BLOCK_LENGTH);
  std::vector<std::byte> tag (16);
  std::int32_t outlen{};

  std::span<const unsigned char> key_span{
    reinterpret_cast<const unsigned char *> (key.get ().data ()), key.size ()
  };

  if (!EVP_EncryptInit_ex (
          ctx, EVP_aes_256_gcm (), nullptr, key_span.data (),
          reinterpret_cast<unsigned char *> (init_vec.data ())))
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  if (!EVP_EncryptUpdate (
          ctx, reinterpret_cast<unsigned char *> (ciphertext.data ()), &outlen,
          reinterpret_cast<const unsigned char *> (data.data ()),
          static_cast<std::int32_t> (data.size ())))
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  std::int32_t final_len;
  if (!EVP_EncryptFinal_ex (
          ctx, reinterpret_cast<unsigned char *> (ciphertext.data () + outlen),
          &final_len))
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  if (!EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_GET_TAG, 16,
                            reinterpret_cast<unsigned char *> (tag.data ())))
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  ciphertext.resize (static_cast<std::size_t> (outlen + final_len));

  return

      std::expected<encrypted_data, std::error_code>{ ftv::encrypted_data{
          std::move (ciphertext), std::move (init_vec), std::move (tag) } };
}

[[nodiscard]] std::expected<encrypted_data, std::error_code>
encrypt (const file &source, const secure_key &key,
         encryption_func fn) noexcept
{
  if (source.size () == 0)
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::invalid_argument)) };
    }

  auto encrypted = fn (source.data (), key);
  if (!encrypted)
    {
      return std::expected<encrypted_data, std::error_code>{ std::unexpected (
          encrypted.error ()) };
    }
  return std::expected<encrypted_data, std::error_code>{ std::move (
      encrypted) };
}

} // namespace ftv
