#include "crypto/secure_key.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

namespace ftv
{

secure_key::secure_key (std::string_view key)
{
  if (key.size () > 32)
    {
      throw std::invalid_argument (std::format (
          "key size must be less than or equal to 32 characters"));
    }
  key_.resize (32, std::byte{ 0 });

  std::ranges::transform (key, key_.begin (),
                          [] (char c) { return std::byte (c); });
}

secure_key::~secure_key () { std::ranges::fill (this->key_, std::byte{ 0 }); }

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

} // namespace ftv
