#pragma once

#include <cstddef>

namespace ftv
{

inline constexpr std::size_t MIN_DIMENSION{ 100 };
inline constexpr std::size_t MAX_DIMENSION{ 4096 };
inline constexpr std::size_t MAX_FPS{ 60 };

struct resolution
{
  std::size_t x{};
  std::size_t y{};
};

} // namespace ftv
