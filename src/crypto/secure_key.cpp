#include "crypto/secure_key.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>

#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/rand.h>

namespace ftv
{

namespace
{

// RFC 9106 second recommended option: 3 passes over 64 MiB
inline constexpr std::uint32_t ARGON2_ITERATIONS{ 3 };
inline constexpr std::uint32_t ARGON2_MEMORY_KIB{ 64 * 1024 };
inline constexpr std::uint32_t ARGON2_LANES{ 4 };

std::vector<std::byte>
random_salt ()
{
  std::vector<std::byte> salt (SALT_SIZE);
  if (RAND_bytes (reinterpret_cast<unsigned char *> (salt.data ()),
                  static_cast<std::int32_t> (salt.size ()))
      != 1)
    {
      throw std::runtime_error ("failed to generate salt");
    }
  return salt;
}

struct kdf_deleter
{
  void
  operator() (EVP_KDF *kdf) const noexcept
  {
    EVP_KDF_free (kdf);
  }

  void
  operator() (EVP_KDF_CTX *ctx) const noexcept
  {
    EVP_KDF_CTX_free (ctx);
  }
};

} // namespace

secure_key::secure_key (std::string_view password)
    : secure_key (password, random_salt ())
{
}

secure_key::secure_key (std::string_view password,
                        std::span<const std::byte> salt)
    : key_ (KEY_SIZE), salt_ (salt.begin (), salt.end ())
{
  if (password.empty ())
    {
      throw std::invalid_argument ("key cannot be empty");
    }
  if (this->salt_.size () != SALT_SIZE)
    {
      throw std::invalid_argument (
          std::format ("salt must be {} bytes", SALT_SIZE));
    }

  const std::unique_ptr<EVP_KDF, kdf_deleter> kdf{ EVP_KDF_fetch (
      nullptr, "ARGON2ID", nullptr) };
  if (!kdf)
    {
      throw std::runtime_error ("argon2id is not available in this openssl");
    }

  const std::unique_ptr<EVP_KDF_CTX, kdf_deleter> ctx{ EVP_KDF_CTX_new (
      kdf.get ()) };
  if (!ctx)
    {
      throw std::runtime_error ("failed to create key derivation context");
    }

  // OSSL_PARAM wants non-const pointers even for inputs
  std::uint32_t iterations = ARGON2_ITERATIONS;
  std::uint32_t memory = ARGON2_MEMORY_KIB;
  std::uint32_t lanes = ARGON2_LANES;
  std::array params{
    OSSL_PARAM_construct_octet_string (OSSL_KDF_PARAM_PASSWORD,
                                       const_cast<char *> (password.data ()),
                                       password.size ()),
    OSSL_PARAM_construct_octet_string (
        OSSL_KDF_PARAM_SALT, this->salt_.data (), this->salt_.size ()),
    OSSL_PARAM_construct_uint32 (OSSL_KDF_PARAM_ITER, &iterations),
    OSSL_PARAM_construct_uint32 (OSSL_KDF_PARAM_ARGON2_MEMCOST, &memory),
    OSSL_PARAM_construct_uint32 (OSSL_KDF_PARAM_ARGON2_LANES, &lanes),
    OSSL_PARAM_construct_end (),
  };

  if (EVP_KDF_derive (ctx.get (),
                      reinterpret_cast<unsigned char *> (this->key_.data ()),
                      this->key_.size (), params.data ())
      != 1)
    {
      throw std::runtime_error ("key derivation failed");
    }
}

// OPENSSL_cleanse is not optimized away like a plain fill can be
secure_key::~secure_key ()
{
  OPENSSL_cleanse (this->key_.data (), this->key_.size ());
}

[[nodiscard]] std::span<const std::byte>
secure_key::get () const noexcept
{
  return this->key_;
}

[[nodiscard]] std::size_t
secure_key::size () const noexcept
{
  return this->key_.size ();
}

[[nodiscard]] std::span<const std::byte>
secure_key::salt () const noexcept
{
  return this->salt_;
}

} // namespace ftv
