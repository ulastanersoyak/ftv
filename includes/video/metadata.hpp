#pragma once

#include "video/resolution.hpp"

#include <cstddef>
#include <string>

namespace ftv
{

struct metadata
{
  std::string filename{};
  std::size_t filesize{};
  std::size_t checksum{};
  std::size_t fps{};
  resolution res{};
};

} // namespace ftv
