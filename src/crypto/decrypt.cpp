#include "crypto/decrypt.hpp"
#include "crypto/evp_cipher_raii.hpp"
#include <cstdint>
#include <functional>
#include <utility>

namespace ftv
{

std::expected<file, std::error_code>
aes_256_gcm_decrypt (const encrypted_data &encrypted, const secure_key &key)
{
  if (key.size () != 32)
    {
      return std::expected<file, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::invalid_argument)) };
    }

  if (encrypted.init_vec ().size () != 12 || encrypted.tag ().size () != 16)
    {
      return std::expected<file, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::invalid_argument)) };
    }

  evp_cipher ctx{};
  std::vector<std::byte> plaintext (encrypted.ciphertext ().size ());
  std::int32_t outlen = 0;
  std::int32_t final_len = 0;

  // initialize decryption
  if (!EVP_DecryptInit_ex (
          ctx, EVP_aes_256_gcm (), nullptr,
          reinterpret_cast<const unsigned char *> (key.get ().data ()),
          reinterpret_cast<const unsigned char *> (
              encrypted.init_vec ().data ())))
    {
      return std::expected<file, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  // set expected tag
  if (!EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_SET_TAG, 16,
                            const_cast<unsigned char *> (
                                reinterpret_cast<const unsigned char *> (
                                    encrypted.tag ().data ()))))
    {
      return std::expected<file, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  // decrypt ciphertext
  if (!EVP_DecryptUpdate (
          ctx, reinterpret_cast<unsigned char *> (plaintext.data ()), &outlen,
          reinterpret_cast<const unsigned char *> (
              encrypted.ciphertext ().data ()),
          static_cast<int> (encrypted.ciphertext ().size ())))
    {
      return std::expected<file, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  // finalize decryption and verify authentication tag
  if (!EVP_DecryptFinal_ex (
          ctx, reinterpret_cast<unsigned char *> (plaintext.data () + outlen),
          &final_len))
    {
      // authentication failed or decryption error
      return std::expected<file, std::error_code>{ std::unexpected (
          std::make_error_code (std::errc::operation_canceled)) };
    }

  // resize to actual decrypted size
  plaintext.resize (static_cast<size_t> (outlen + final_len));

  const std::filesystem::path temp_path
      = std::filesystem::temp_directory_path () / "decrypted_temp";

  // create file object
  const file decrypted_file (temp_path, plaintext);

  return std::expected<file, std::error_code>{ std::move (decrypted_file) };
}

[[nodiscard]] std::expected<file, std::error_code>
decrypt (const encrypted_data &data, const secure_key &key,
         decryption_func fn) noexcept
{
  auto decrypted = fn (data, key);
  if (!decrypted)
    {

      return std::expected<file, std::error_code>{ std::unexpected (
          decrypted.error ()) };
    }
  return std::expected<file, std::error_code>{ std::move (decrypted) };
}

} // namespace ftv
