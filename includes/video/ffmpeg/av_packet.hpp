#pragma once

extern "C"
{
#include <libavcodec/packet.h>
}

#include <system_error>

namespace ftv
{

class av_packet
{
public:
  av_packet ();
  ~av_packet ();

  av_packet (const av_packet &) = delete;
  av_packet &operator= (const av_packet &) = delete;

  av_packet (av_packet &&other) noexcept;
  av_packet &operator= (av_packet &&other) noexcept;

  [[nodiscard]] AVPacket *get () noexcept;
  [[nodiscard]] const AVPacket *get () const noexcept;

  operator AVPacket * () noexcept;
  operator const AVPacket * () const noexcept;

  [[nodiscard]] std::error_code unref () noexcept;

private:
  AVPacket *packet_{ nullptr };
};

} // namespace ftv
