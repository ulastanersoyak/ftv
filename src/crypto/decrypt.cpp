#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/evp_cipher_raii.hpp"

#include <algorithm>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <utility>

namespace ftv
{

namespace
{

// EVP takes int lengths, so larger inputs are fed in chunks
inline constexpr std::size_t EVP_CHUNK_SIZE{ 1UZ << 30 };

std::unexpected<std::error_code>
fail (std::errc error)
{
  return std::unexpected (std::make_error_code (error));
}

} // namespace

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
aes_256_gcm_decrypt (const encrypted_data &encrypted,
                     const secure_key &key) noexcept
{
  try
    {
      if (key.size () != KEY_SIZE
          || encrypted.init_vec ().size () != AES_GCM_IV_SIZE
          || encrypted.tag ().size () != AES_GCM_TAG_SIZE)
        {
          return fail (std::errc::invalid_argument);
        }

      evp_cipher ctx{};
      const std::span<const std::byte> ciphertext = encrypted.ciphertext ();
      std::vector<std::byte> plaintext (ciphertext.size ());

      if (!EVP_DecryptInit_ex (
              ctx, EVP_aes_256_gcm (), nullptr,
              reinterpret_cast<const unsigned char *> (key.get ().data ()),
              reinterpret_cast<const unsigned char *> (
                  encrypted.init_vec ().data ())))
        {
          return fail (std::errc::operation_canceled);
        }

      // set expected tag
      if (!EVP_CIPHER_CTX_ctrl (
              ctx, EVP_CTRL_GCM_SET_TAG,
              static_cast<std::int32_t> (encrypted.tag ().size ()),
              const_cast<unsigned char *> (
                  reinterpret_cast<const unsigned char *> (
                      encrypted.tag ().data ()))))
        {
          return fail (std::errc::operation_canceled);
        }

      std::size_t written = 0;
      for (std::size_t offset = 0; offset < ciphertext.size ();
           offset += EVP_CHUNK_SIZE)
        {
          const auto chunk = ciphertext.subspan (
              offset, std::min (EVP_CHUNK_SIZE, ciphertext.size () - offset));
          std::int32_t outlen = 0;
          if (!EVP_DecryptUpdate (
                  ctx,
                  reinterpret_cast<unsigned char *> (plaintext.data ()
                                                     + written),
                  &outlen,
                  reinterpret_cast<const unsigned char *> (chunk.data ()),
                  static_cast<std::int32_t> (chunk.size ())))
            {
              return fail (std::errc::operation_canceled);
            }
          written += static_cast<std::size_t> (outlen);
        }

      // finalize decryption and verify authentication tag
      std::int32_t final_len = 0;
      if (!EVP_DecryptFinal_ex (
              ctx,
              reinterpret_cast<unsigned char *> (plaintext.data () + written),
              &final_len))
        {
          return fail (std::errc::bad_message);
        }
      written += static_cast<std::size_t> (final_len);

      plaintext.resize (written);
      return plaintext;
    }
  catch (const std::bad_alloc &)
    {
      return fail (std::errc::not_enough_memory);
    }
  catch (...)
    {
      return fail (std::errc::operation_canceled);
    }
}

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
decrypt (const encrypted_data &data, std::string_view password,
         decryption_func fn) noexcept
{
  try
    {
      const secure_key key{ password, data.salt () };
      return fn (data, key);
    }
  catch (const std::invalid_argument &)
    {
      return fail (std::errc::invalid_argument);
    }
  catch (const std::bad_alloc &)
    {
      return fail (std::errc::not_enough_memory);
    }
  catch (...)
    {
      return fail (std::errc::operation_canceled);
    }
}

} // namespace ftv
