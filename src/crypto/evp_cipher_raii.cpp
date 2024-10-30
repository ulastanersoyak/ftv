#include "crypto/evp_cipher_raii.hpp"

#include <stdexcept>
#include <utility>

namespace ftv
{

evp_cipher::evp_cipher () : ctx_{ EVP_CIPHER_CTX_new () }
{
  if (!this->ctx_)
    {
      throw std::runtime_error ("Failed to create cipher context");
    }
}

evp_cipher::evp_cipher (EVP_CIPHER_CTX *ctx) : ctx_{ ctx }
{
  if (!this->ctx_)
    {
      throw std::runtime_error ("Invalid cipher context");
    }
}

evp_cipher::~evp_cipher ()
{
  if (this->ctx_)
    {
      EVP_CIPHER_CTX_free (ctx_);
    }
}

evp_cipher::evp_cipher (evp_cipher &&other) noexcept
    : ctx_{ std::exchange (other.ctx_, nullptr) }
{
}

evp_cipher &
evp_cipher::operator= (evp_cipher &&other) noexcept
{
  if (this != &other)
    {
      if (this->ctx_)
        {
          EVP_CIPHER_CTX_free (this->ctx_);
        }
      this->ctx_ = std::exchange (other.ctx_, nullptr);
    }
  return *this;
}

[[nodiscard]] EVP_CIPHER_CTX *
evp_cipher::ctx () noexcept
{
  return this->ctx_;
}

[[nodiscard]] const EVP_CIPHER_CTX *
evp_cipher::ctx () const noexcept
{
  return this->ctx_;
}

[[nodiscard]] evp_cipher::operator EVP_CIPHER_CTX * () noexcept
{
  return this->ctx_;
}

[[nodiscard]] evp_cipher::operator const EVP_CIPHER_CTX * () const noexcept
{
  return this->ctx_;
}

} // namespace ftv
