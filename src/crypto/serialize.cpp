#include "crypto/serialize.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/secure_key.hpp"
#include "util/endian.hpp"

#include <cstdint>
#include <optional>
#include <utility>

namespace ftv
{

namespace
{

void
write_field (std::vector<std::byte> &out, std::span<const std::byte> field)
{
  put_le (out, static_cast<std::uint32_t> (field.size ()));
  out.insert (out.end (), field.begin (), field.end ());
}

// reads a size-prefixed field that must be exactly expected_size bytes long
std::optional<std::vector<std::byte>>
read_field (std::span<const std::byte> data, std::size_t &offset,
            std::size_t expected_size)
{
  if (data.size () - offset < sizeof (std::uint32_t))
    {
      return std::nullopt;
    }
  const auto size = get_le<std::uint32_t> (data, offset);
  offset += sizeof (std::uint32_t);

  if (size != expected_size || data.size () - offset < size)
    {
      return std::nullopt;
    }
  const auto field = data.subspan (offset, size);
  offset += size;
  return std::vector<std::byte> (field.begin (), field.end ());
}

} // namespace

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
serialize_encrypted_data (const encrypted_data &data) noexcept
{
  if (data.salt ().size () != SALT_SIZE
      || data.init_vec ().size () != AES_GCM_IV_SIZE
      || data.tag ().size () != AES_GCM_TAG_SIZE)
    {
      return std::unexpected (
          std::make_error_code (std::errc::invalid_argument));
    }

  try
    {
      std::vector<std::byte> serialized{};
      serialized.reserve (3 * sizeof (std::uint32_t) + data.salt ().size ()
                          + data.init_vec ().size () + data.tag ().size ()
                          + data.ciphertext ().size ());

      write_field (serialized, data.salt ());
      write_field (serialized, data.init_vec ());
      write_field (serialized, data.tag ());
      serialized.insert (serialized.end (), data.ciphertext ().begin (),
                         data.ciphertext ().end ());

      return serialized;
    }
  catch (const std::exception &)
    {
      return std::unexpected (
          std::make_error_code (std::errc::not_enough_memory));
    }
}

[[nodiscard]]
std::expected<encrypted_data, std::error_code>
deserialize_encrypted_data (
    std::span<const std::byte> serialized_data) noexcept
{
  try
    {
      std::size_t offset = 0;
      auto salt = read_field (serialized_data, offset, SALT_SIZE);
      auto init_vec = read_field (serialized_data, offset, AES_GCM_IV_SIZE);
      auto tag = read_field (serialized_data, offset, AES_GCM_TAG_SIZE);
      if (!salt || !init_vec || !tag)
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }

      // the remaining data is the ciphertext
      const auto ciphertext = serialized_data.subspan (offset);

      return encrypted_data (
          std::vector<std::byte> (ciphertext.begin (), ciphertext.end ()),
          std::move (*init_vec), std::move (*tag), std::move (*salt));
    }
  catch (const std::exception &)
    {
      return std::unexpected (
          std::make_error_code (std::errc::not_enough_memory));
    }
}

} // namespace ftv
