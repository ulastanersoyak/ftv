#include "crypto/secure_key.hpp"

#include <algorithm>
#include <format>
#include <iterator>
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

  key_.reserve (32);

  std::ranges::transform (key, std::back_inserter (this->key_),
                          [] (char c) { return std::byte (c); });

  // pad with zeros up to 32 bytes if needed
  if (key.size () < 32)
    {
      key_.resize (32, std::byte{ 0 });
    }
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
