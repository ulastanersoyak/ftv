#include "video/av_codec_ctx.hpp"
#include <stdexcept>

namespace ftv
{

av_codec_ctx::av_codec_ctx (AVCodecID id) : codec_id_ (id)
{
  const AVCodec *codec = avcodec_find_encoder (codec_id_);
  if (!codec)
    {
      throw std::runtime_error ("Failed to find encoder");
    }

  codec_ctx_ = avcodec_alloc_context3 (codec);
  if (!codec_ctx_)
    {
      throw std::runtime_error ("Failed to allocate encoder context");
    }
}

av_codec_ctx::~av_codec_ctx ()
{
  if (codec_ctx_)
    {
      avcodec_free_context (&codec_ctx_);
    }
}

av_codec_ctx::av_codec_ctx (av_codec_ctx &&other) noexcept
    : codec_ctx_ (other.codec_ctx_), codec_id_ (other.codec_id_)
{
  other.codec_ctx_ = nullptr;
}

av_codec_ctx &
av_codec_ctx::operator= (av_codec_ctx &&other) noexcept
{
  if (this != &other)
    {
      if (codec_ctx_)
        {
          avcodec_free_context (&codec_ctx_);
        }
      codec_id_ = other.codec_id_;
      codec_ctx_ = other.codec_ctx_;
      other.codec_ctx_ = nullptr;
    }
  return *this;
}

AVCodecContext *
av_codec_ctx::get () noexcept
{
  return codec_ctx_;
}

const AVCodecContext *
av_codec_ctx::get () const noexcept
{
  return codec_ctx_;
}

av_codec_ctx::operator AVCodecContext * () noexcept { return codec_ctx_; }

av_codec_ctx::operator const AVCodecContext * () const noexcept
{
  return codec_ctx_;
}

} // namespace ftv
