#include "video/header.hpp"
#include "util/endian.hpp"

#include <algorithm>

namespace ftv
{

namespace
{

inline constexpr std::array MAGIC{ std::byte{ 'F' }, std::byte{ 'T' },
                                   std::byte{ 'V' } };
inline constexpr std::byte FORMAT_VERSION{ 1 };

} // namespace

[[nodiscard]] std::vector<std::byte>
encode_header (const header &h)
{
  std::vector<std::byte> bytes (MAGIC.begin (), MAGIC.end ());
  bytes.push_back (FORMAT_VERSION);
  bytes.push_back (static_cast<std::byte> (h.block_size));
  bytes.insert (bytes.end (), 3, std::byte{ 0 });
  put_le (bytes, h.payload_size);
  return rs_encode_block (bytes);
}

[[nodiscard]] std::expected<header, std::error_code>
decode_header (std::span<const std::byte> encoded) noexcept
{
  if (encoded.size () != ENCODED_HEADER_SIZE)
    {
      return std::unexpected (
          std::make_error_code (std::errc::invalid_argument));
    }

  const auto bytes = rs_decode_block (encoded);
  if (!bytes)
    {
      return std::unexpected (bytes.error ());
    }

  const std::span<const std::byte> view{ *bytes };
  if (!std::ranges::equal (view.first (MAGIC.size ()), MAGIC))
    {
      return std::unexpected (std::make_error_code (std::errc::bad_message));
    }
  if (view[3] != FORMAT_VERSION)
    {
      return std::unexpected (std::make_error_code (std::errc::not_supported));
    }

  const auto block_size = std::to_integer<std::size_t> (view[4]);
  const auto payload_size = get_le<std::uint64_t> (view, 8);
  if (std::ranges::find (BLOCK_SIZES, block_size) == BLOCK_SIZES.end ()
      || payload_size > MAX_PAYLOAD_SIZE)
    {
      return std::unexpected (std::make_error_code (std::errc::bad_message));
    }

  return header{ block_size, payload_size };
}

} // namespace ftv
