#include "file/file.hpp"

#include <expected>
#include <functional>

namespace ftv
{

using encryption_func
    = std::function<std::expected<std::vector<std::byte>, std::error_code> (
        std::span<const std::byte> data)>;

[[nodiscard]] std::expected<file, std::error_code>
encrypt (const file &source, encryption_func fn);

} // namespace ftv
