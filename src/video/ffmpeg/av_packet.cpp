#include "video/ffmpeg/av_packet.hpp"
#include <stdexcept>

namespace ftv
{

av_packet::av_packet ()
{
  this->packet_ = av_packet_alloc ();
  if (!this->packet_)
    {
      throw std::runtime_error ("failed to allocate packet");
    }
}

av_packet::~av_packet ()
{
  if (this->packet_)
    {
      av_packet_free (&this->packet_);
      this->packet_ = nullptr;
    }
}

av_packet::av_packet (av_packet &&other) noexcept : packet_ (other.packet_)
{
  other.packet_ = nullptr;
}

av_packet &
av_packet::operator= (av_packet &&other) noexcept
{
  if (this != &other)
    {
      if (this->packet_)
        {
          av_packet_free (&packet_);
        }
      this->packet_ = other.packet_;
      other.packet_ = nullptr;
    }
  return *this;
}

[[nodiscard]] AVPacket *
av_packet::get () noexcept
{
  return this->packet_;
}

[[nodiscard]] const AVPacket *
av_packet::get () const noexcept
{
  return this->packet_;
}

av_packet::operator AVPacket * () noexcept { return this->packet_; }

av_packet::operator const AVPacket * () const noexcept
{
  return this->packet_;
}

[[nodiscard]] std::error_code
av_packet::unref () noexcept
{
  if (!this->packet_)
    {
      return std::make_error_code (std::errc::invalid_argument);
    }
  av_packet_unref (this->packet_);
  return {};
}

} // namespace ftv
