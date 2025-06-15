#include "crypto/encrypt.hpp"
#include "crypto/evp_cipher_raii.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <new>
#include <stdexcept>
#include <utility>

#include <openssl/evp.h>
#include <openssl/rand.h>

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

[[nodiscard]] std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, const secure_key &key) noexcept
{
  try
    {
      if (key.size () != KEY_SIZE)
        {
          return fail (std::errc::invalid_argument);
        }

      std::vector<std::byte> init_vec (AES_GCM_IV_SIZE);
      if (RAND_bytes (reinterpret_cast<unsigned char *> (init_vec.data ()),
                      static_cast<std::int32_t> (init_vec.size ()))
          != 1)
        {
          return fail (std::errc::operation_canceled);
        }

      evp_cipher ctx{};
      // gcm is a stream mode, the ciphertext is as long as the plaintext
      std::vector<std::byte> ciphertext (data.size ());
      std::vector<std::byte> tag (AES_GCM_TAG_SIZE);

      if (!EVP_EncryptInit_ex (
              ctx, EVP_aes_256_gcm (), nullptr,
              reinterpret_cast<const unsigned char *> (key.get ().data ()),
              reinterpret_cast<const unsigned char *> (init_vec.data ())))
        {
          return fail (std::errc::operation_canceled);
        }

      std::size_t written = 0;
      for (std::size_t offset = 0; offset < data.size ();
           offset += EVP_CHUNK_SIZE)
        {
          const auto chunk = data.subspan (
              offset, std::min (EVP_CHUNK_SIZE, data.size () - offset));
          std::int32_t outlen = 0;
          if (!EVP_EncryptUpdate (
                  ctx,
                  reinterpret_cast<unsigned char *> (ciphertext.data ()
                                                     + written),
                  &outlen,
                  reinterpret_cast<const unsigned char *> (chunk.data ()),
                  static_cast<std::int32_t> (chunk.size ())))
            {
              return fail (std::errc::operation_canceled);
            }
          written += static_cast<std::size_t> (outlen);
        }

      std::int32_t final_len = 0;
      if (!EVP_EncryptFinal_ex (
              ctx,
              reinterpret_cast<unsigned char *> (ciphertext.data () + written),
              &final_len))
        {
          return fail (std::errc::operation_canceled);
        }
      written += static_cast<std::size_t> (final_len);

      if (!EVP_CIPHER_CTX_ctrl (
              ctx, EVP_CTRL_GCM_GET_TAG,
              static_cast<std::int32_t> (tag.size ()),
              reinterpret_cast<unsigned char *> (tag.data ())))
        {
          return fail (std::errc::operation_canceled);
        }

      ciphertext.resize (written);

      return encrypted_data{
        std::move (ciphertext), std::move (init_vec), std::move (tag),
        std::vector<std::byte> (key.salt ().begin (), key.salt ().end ())
      };
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

[[nodiscard]] std::expected<encrypted_data, std::error_code>
encrypt (std::span<const std::byte> data, std::string_view password,
         encryption_func fn) noexcept
{
  try
    {
      const secure_key key{ password };
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
