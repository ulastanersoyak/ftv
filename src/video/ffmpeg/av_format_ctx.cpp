#include <stdexcept>

#include "video/ffmpeg/av_format_ctx.hpp"

namespace ftv
{

av_format_ctx::av_format_ctx (const std::filesystem::path &path)
{
  std::int32_t ret = avformat_alloc_output_context2 (&this->fmt_ctx_, nullptr,
                                                     nullptr, path.c_str ());
  if (ret < 0)
    {
      throw std::runtime_error ("could not create output context");
    }
}

av_format_ctx::~av_format_ctx ()
{
  if (this->fmt_ctx_)
    {
      avformat_free_context (this->fmt_ctx_);
      this->fmt_ctx_ = nullptr;
    }
}

av_format_ctx::av_format_ctx (av_format_ctx &&other) noexcept
    : fmt_ctx_ (other.fmt_ctx_)
{
  other.fmt_ctx_ = nullptr;
}

av_format_ctx &
av_format_ctx::operator= (av_format_ctx &&other) noexcept
{
  if (this != &other)
    {
      if (this->fmt_ctx_)
        {
          avformat_free_context (this->fmt_ctx_);
        }
      this->fmt_ctx_ = other.fmt_ctx_;
      other.fmt_ctx_ = nullptr;
    }
  return *this;
}

AVFormatContext *
av_format_ctx::get () noexcept
{
  return this->fmt_ctx_;
}

const AVFormatContext *
av_format_ctx::get () const noexcept
{
  return this->fmt_ctx_;
}

av_format_ctx::operator AVFormatContext * () noexcept
{
  return this->fmt_ctx_;
}

av_format_ctx::operator const AVFormatContext * () const noexcept
{
  return this->fmt_ctx_;
}

} // namespace ftv
