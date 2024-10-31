#pragma once

#include <cstddef>

namespace ftv
{

struct position
{
  std::size_t x{};
  std::size_t y{};
};

struct frame_position
{
  position pos{};
  std::size_t frame_index{};
};

} // namespace ftv
