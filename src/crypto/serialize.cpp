#include "crypto/serialize.hpp"

#include <algorithm>
#include <cstring>

namespace ftv
{

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
serialize_encrypted_data (const encrypted_data &data)
{
  if (data.init_vec.empty () || data.tag.empty ())
    {
      return std::expected<std::vector<std::byte>, std::error_code> (
          std::unexpected (
              std::make_error_code (std::errc::invalid_argument)));
    }

  try
    {
      const size_t total_size = 4 +                      // iv size field
                                data.init_vec.size () +  // iv data
                                4 +                      // tag size field
                                data.tag.size () +       // tag data
                                data.ciphertext.size (); // ciphertext

      std::vector<std::byte> serialized{};
      serialized.reserve (total_size);

      // add iv size
      const auto iv_size = static_cast<std::uint32_t> (data.init_vec.size ());
      std::ranges::copy (
          std::span{ reinterpret_cast<const std::byte *> (&iv_size), 4 },
          std::back_inserter (serialized));

      // add iv data
      std::ranges::copy (data.init_vec, std::back_inserter (serialized));

      // add tag size
      const auto tag_size = static_cast<std::uint32_t> (data.tag.size ());
      std::ranges::copy (
          std::span{ reinterpret_cast<const std::byte *> (&tag_size), 4 },
          std::back_inserter (serialized));

      // add tag data
      std::ranges::copy (data.tag, std::back_inserter (serialized));

      // add ciphertext
      std::ranges::copy (data.ciphertext, std::back_inserter (serialized));

      return std::expected<std::vector<std::byte>, std::error_code> (
          std::move (serialized));
    }
  catch (const std::exception &)
    {
      return std::expected<std::vector<std::byte>, std::error_code> (
          std::unexpected (
              std::make_error_code (std::errc::not_enough_memory)));
    }
}

[[nodiscard]]
std::expected<encrypted_data, std::error_code>
deserialize_encrypted_data (std::span<const std::byte> serialized_data)
{
  // need at least 8 bytes for the size fields
  if (serialized_data.size () < 8)
    {
      return std::expected<encrypted_data, std::error_code> (std::unexpected (
          std::make_error_code (std::errc::invalid_argument)));
    }

  try
    {
      size_t offset = 0;

      // read iv size
      std::uint32_t iv_size;
      std::memcpy (&iv_size, serialized_data.data () + offset,
                   sizeof (iv_size));
      offset += sizeof (iv_size);

      // validate iv size and remaining data
      if (iv_size == 0
          || serialized_data.size ()
                 < offset + iv_size + sizeof (std::uint32_t))
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }

      // extract iv data
      std::vector<std::byte> init_vec (
          serialized_data.begin () + static_cast<ssize_t> (offset),
          serialized_data.begin () + static_cast<ssize_t> (offset) + iv_size);
      offset += iv_size;

      // read tag size
      std::uint32_t tag_size;
      std::memcpy (&tag_size, serialized_data.data () + offset,
                   sizeof (tag_size));
      offset += sizeof (tag_size);

      // validate tag size and remaining data
      if (tag_size == 0 || serialized_data.size () < offset + tag_size)
        {
          return std::expected<encrypted_data, std::error_code> (
              std::unexpected (
                  std::make_error_code (std::errc::invalid_argument)));
        }

      // extract tag data
      std::vector<std::byte> tag (
          serialized_data.begin () + static_cast<ssize_t> (offset),
          serialized_data.begin () + static_cast<ssize_t> (offset) + tag_size);
      offset += tag_size;

      // extract remaining data as ciphertext
      std::vector<std::byte> ciphertext (serialized_data.begin ()
                                             + static_cast<ssize_t> (offset),
                                         serialized_data.end ());

      return std::expected<encrypted_data, std::error_code> (
          encrypted_data{ .ciphertext = std::move (ciphertext),
                          .init_vec = std::move (init_vec),
                          .tag = std::move (tag) });
    }
  catch (const std::exception &)
    {
      return std::expected<encrypted_data, std::error_code> (std::unexpected (
          std::make_error_code (std::errc::not_enough_memory)));
    }
}

} // namespace ftv
