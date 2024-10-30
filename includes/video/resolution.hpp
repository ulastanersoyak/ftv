#pragma once

#include <cstddef>

namespace ftv
{

inline constexpr std::size_t MAX_RESOLUTION{ 1920 };
inline constexpr std::size_t MAX_FPS{ 60 };

struct resolution
{
  std::size_t x{};
  std::size_t y{};
};

} // namespace ftv
