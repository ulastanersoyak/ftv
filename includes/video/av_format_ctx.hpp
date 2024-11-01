#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <filesystem>

namespace ftv
{

class av_format_ctx
{
public:
  av_format_ctx (const std::filesystem::path &path);
  ~av_format_ctx ();

  av_format_ctx (const av_format_ctx &) = delete;
  av_format_ctx &operator= (const av_format_ctx &) = delete;

  av_format_ctx (av_format_ctx &&other) noexcept;
  av_format_ctx &operator= (av_format_ctx &&other) noexcept;

  [[nodiscard]] AVFormatContext *get () noexcept;
  [[nodiscard]] const AVFormatContext *get () const noexcept;

  operator AVFormatContext * () noexcept;
  operator const AVFormatContext * () const noexcept;

public:
private:
  AVFormatContext *fmt_ctx_{ nullptr };
};

} // namespace ftv
