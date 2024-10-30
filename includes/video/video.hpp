#pragma once

#include "crypto/encrypt.hpp"
#include "file/file.hpp"
#include "video/metadata.hpp"
#include "video/resolution.hpp"

#include <vector>

namespace ftv
{

class video
{
public:
  video (const file &file, const secure_key &key, resolution res,
         std::size_t fps);

private:
  resolution res_{};
  std::size_t fps_{};
  metadata metadata_{};
  std::vector<std::byte> data_{};
};

} // namespace ftv
