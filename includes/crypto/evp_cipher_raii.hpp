#pragma once

#include <openssl/evp.h>

namespace ftv
{

class evp_cipher
{
public:
  evp_cipher ();
  explicit evp_cipher (EVP_CIPHER_CTX *ctx);

  evp_cipher (const evp_cipher &) = delete;
  evp_cipher &operator= (const evp_cipher &) = delete;

  ~evp_cipher ();

  evp_cipher (evp_cipher &&other) noexcept;

  evp_cipher &operator= (evp_cipher &&other) noexcept;

  [[nodiscard]] EVP_CIPHER_CTX *ctx () noexcept;
  [[nodiscard]] const EVP_CIPHER_CTX *ctx () const noexcept;

  [[nodiscard]] operator EVP_CIPHER_CTX * () noexcept;
  [[nodiscard]] operator const EVP_CIPHER_CTX * () const noexcept;

private:
  EVP_CIPHER_CTX *ctx_;
};

} // namespace ftv
