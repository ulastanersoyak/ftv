#include "video/av_frame.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace ftv
{

av_frame::av_frame (const resolution &res) : frame_ (av_frame_alloc ())
{
  this->frame_->width = static_cast<std::int32_t> (res.x);
  this->frame_->height = static_cast<std::int32_t> (res.y);
  this->frame_->format = AV_PIX_FMT_RGBA;

  std::int32_t ret = av_frame_get_buffer (this->frame_, 0);
  if (ret < 0)
    {
      throw std::runtime_error ("failed to allocate frame buffer");
    }

  ret = av_frame_make_writable (this->frame_);
  if (ret < 0)
    {
      throw std::runtime_error ("failed to make frame writable");
    }
}

av_frame::~av_frame ()
{
  if (this->frame_)
    {
      av_frame_free (&this->frame_);
    }
}

av_frame::av_frame (av_frame &&other) noexcept : frame_ (other.frame_)
{
  other.frame_ = nullptr;
}

av_frame &
av_frame::operator= (av_frame &&other) noexcept
{
  if (this != &other)
    {
      if (this->frame_)
        {
          av_frame_free (&this->frame_);
        }
      this->frame_ = other.frame_;
      other.frame_ = nullptr;
    }
  return *this;
}

[[nodiscard]] AVFrame *
av_frame::get () noexcept
{
  return this->frame_;
}

[[nodiscard]] const AVFrame *
av_frame::get () const noexcept
{
  return this->frame_;
}

av_frame::operator AVFrame * () noexcept { return this->frame_; }
av_frame::operator const AVFrame * () const noexcept { return this->frame_; }

[[nodiscard]] std::error_code
av_frame::reset () const noexcept
{
  if (!frame_ || !frame_->data[0])
    {
      return std::make_error_code (std::errc::invalid_argument);
    }
  av_frame_unref (this->frame_);
  return {};
}

} // namespace ftv
