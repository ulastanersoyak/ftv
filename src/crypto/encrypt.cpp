#include "crypto/encrypt.hpp"

#include <algorithm>
#include <expected>
#include <memory>
#include <utility>

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace ftv
{

std::expected<encrypted_data, std::error_code>
aes_256_gcm (std::span<const std::byte> data, std::string_view key)
{
  if (key.size () == 0 || key.size () != 32
      || std::ranges::all_of (key, [] (char c) { return c == '0'; }))
    {
      return std::unexpected (
          std::make_error_code (std::errc::invalid_argument));
    }

  // generate random iv (12 bytes for GCM)
  std::vector<std::byte> iv (12);
  if (RAND_bytes (reinterpret_cast<unsigned char *> (iv.data ()),
                  static_cast<std::int32_t> (iv.size ()))
      != 1)
    {
      return std::unexpected (
          std::make_error_code (std::errc::operation_canceled));
    }

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new ();
  std::vector<std::byte> ciphertext (data.size () + EVP_MAX_BLOCK_LENGTH);
  std::vector<std::byte> tag (16);
  std::int32_t outlen;

  std::span<const unsigned char> key_span{
    reinterpret_cast<const unsigned char *> (key.data ()), key.size ()
  };

  EVP_EncryptInit_ex (ctx, EVP_aes_256_gcm (), nullptr, key_span.data (),
                      reinterpret_cast<unsigned char *> (iv.data ()));

  // encrypt
  EVP_EncryptUpdate (
      ctx, reinterpret_cast<unsigned char *> (ciphertext.data ()), &outlen,
      reinterpret_cast<const unsigned char *> (data.data ()),
      static_cast<std::int32_t> (data.size ()));

  // finalize
  std::int32_t final_len;
  EVP_EncryptFinal_ex (
      ctx, reinterpret_cast<unsigned char *> (ciphertext.data () + outlen),
      &final_len);

  // get the tag
  EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_GET_TAG, 16,
                       reinterpret_cast<unsigned char *> (tag.data ()));

  EVP_CIPHER_CTX_free (ctx);
  ciphertext.resize (static_cast<std::size_t> (outlen + final_len));

  return ftv::encrypted_data{ .ciphertext = std::move (ciphertext),
                              .iv = std::move (iv),
                              .tag = std::move (tag) };
}

[[nodiscard]] constexpr static std::expected<
    std::unique_ptr<std::vector<std::byte>>, std::error_code>
serialize_encrypted_data (const encrypted_data &data)
{
  if (data.iv.empty () || data.tag.empty ())
    {
      return std::unexpected (
          std::make_error_code (std::errc::invalid_argument));
    }

  try
    {
      // serialized encryopted data:
      // [iv size (4 bytes)][iv data][tag size (4 bytes)][tag data][ciphertext]
      const size_t total_size = 4 +                      // iv size field
                                data.iv.size () +        // iv data
                                4 +                      // tag size field
                                data.tag.size () +       // tag data
                                data.ciphertext.size (); // ciphertext

      auto serialized = std::make_unique<std::vector<std::byte>> ();
      serialized->reserve (total_size);

      // add iv size
      const auto iv_size = static_cast<std::uint32_t> (data.iv.size ());
      std::ranges::copy (
          std::span{ reinterpret_cast<const std::byte *> (&iv_size), 4 },
          std::back_inserter (*serialized));

      // add iv data
      std::ranges::copy (data.iv, std::back_inserter (*serialized));

      // add tag size
      const auto tag_size = static_cast<std::uint32_t> (data.tag.size ());
      std::ranges::copy (
          std::span{ reinterpret_cast<const std::byte *> (&tag_size), 4 },
          std::back_inserter (*serialized));

      // add tag data
      std::ranges::copy (data.tag, std::back_inserter (*serialized));

      // add ciphertext
      std::ranges::copy (data.ciphertext, std::back_inserter (*serialized));

      return serialized;
    }
  catch (const std::exception &)
    {
      return std::unexpected (
          std::make_error_code (std::errc::not_enough_memory));
    }
}

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
encrypt (const file &source, std::string_view key, encryption_func fn)
{
  if (source.size () == 0)
    {
      return std::unexpected (
          std::make_error_code (std::errc::invalid_argument));
    }

  auto encrypted = fn (source.data (), key);
  if (!encrypted)
    {
      return std::unexpected (encrypted.error ());
    }

  auto serialized = serialize_encrypted_data (*encrypted);
  if (!serialized)
    {
      return std::unexpected (serialized.error ());
    }

  return std::vector<std::byte> (std::move (**serialized));
}

} // namespace ftv
