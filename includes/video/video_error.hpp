#pragma once

#include <stdexcept>

namespace ftv
{

enum class video_error_code
{
  encryption_failed,
  serialization_failed,
  invalid_resolution,
  invalid_fps
};

class video_error : public std::runtime_error
{
public:
  video_error (const std::string &message, video_error_code code)
      : std::runtime_error (message), code_ (code)
  {
  }

  [[nodiscard]] video_error_code
  code () const noexcept
  {
    return code_;
  }

private:
  video_error_code code_;
};

} // namespace ftv
