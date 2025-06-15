#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace ftv
{

inline constexpr std::size_t KEY_SIZE{ 32 };
inline constexpr std::size_t SALT_SIZE{ 16 };

// a 256-bit key derived from a password with argon2id. the salt is not
// secret and has to be stored next to the ciphertext to derive the same key
// again
class secure_key
{

public:
  // derives a key using a fresh random salt
  explicit secure_key (std::string_view password);

  // derives a key using the given salt
  secure_key (std::string_view password, std::span<const std::byte> salt);

  secure_key (const secure_key &) = delete;
  secure_key &operator= (const secure_key &) = delete;

  ~secure_key ();

  [[nodiscard]] std::span<const std::byte> get () const noexcept;

  [[nodiscard]] std::size_t size () const noexcept;

  [[nodiscard]] std::span<const std::byte> salt () const noexcept;

private:
  std::vector<std::byte> key_{};
  std::vector<std::byte> salt_{};
};

} // namespace ftv
