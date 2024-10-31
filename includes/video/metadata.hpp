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
  std::size_t fps{};
  std::size_t width{};
  std::size_t height{};
};

} // namespace ftv
