#include "crypto/checksum.hpp"

#include <functional>
#include <string_view>

namespace ftv
{

[[nodiscard]] std::size_t
hash (std::span<const std::byte> data)
{
  return std::hash<std::string_view>{}(std::string_view (
      reinterpret_cast<const char *> (data.data ()), data.size ()));
}

} // namespace ftv
