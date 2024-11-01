#pragma once

#include "video/resolution.hpp"
#include <system_error>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

namespace ftv
{

class av_frame
{
public:
  av_frame (const resolution &res);
  ~av_frame ();

  av_frame (const av_frame &) = delete;
  av_frame &operator= (const av_frame &) = delete;

  av_frame (av_frame &&other) noexcept;
  av_frame &operator= (av_frame &&other) noexcept;

  [[nodiscard]] AVFrame *get () noexcept;
  [[nodiscard]] const AVFrame *get () const noexcept;

  operator AVFrame * () noexcept;
  operator const AVFrame * () const noexcept;

  [[nodiscard]] std::error_code reset () const noexcept;

private:
  AVFrame *frame_{ nullptr };
};

} // namespace ftv
