#pragma once

#include <cstddef>
#include <span>

namespace ftv
{

[[nodiscard]] std::size_t hash (std::span<std::byte> data);

} // namespace ftv
