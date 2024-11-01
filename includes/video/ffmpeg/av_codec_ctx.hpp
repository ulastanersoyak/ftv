#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace ftv
{

class av_codec_ctx
{
public:
  av_codec_ctx () = default;
  av_codec_ctx (AVCodecID id);
  ~av_codec_ctx ();

  av_codec_ctx (const av_codec_ctx &) = delete;
  av_codec_ctx &operator= (const av_codec_ctx &) = delete;

  av_codec_ctx (av_codec_ctx &&other) noexcept;
  av_codec_ctx &operator= (av_codec_ctx &&other) noexcept;

  [[nodiscard]] AVCodecContext *get () noexcept;
  [[nodiscard]] const AVCodecContext *get () const noexcept;

  operator AVCodecContext * () noexcept;
  operator const AVCodecContext * () const noexcept;

public:
private:
  AVCodecContext *codec_ctx_{ nullptr };
  AVCodecID codec_id_{};
};

} // namespace ftv
