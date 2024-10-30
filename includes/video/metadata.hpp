#pragma once

#include <cstddef>
#include <string>

namespace ftv
{

struct metadata
{
  std::string filename{};
  std::size_t filesize{};
  std::size_t checksum{};
};

} // namespace ftv
