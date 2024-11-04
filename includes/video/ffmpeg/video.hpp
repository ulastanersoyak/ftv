#pragma once

#include "video/ffmpeg/av_codec_ctx.hpp"
#include "video/ffmpeg/av_format_ctx.hpp"
#include "video/ffmpeg/av_frame.hpp"
#include "video/ffmpeg/av_packet.hpp"
#include "video/metadata.hpp"
#include "video/pixel.hpp"

#include <cstddef>
#include <filesystem>
#include <span>

namespace ftv
{

class video
{
public:
  explicit video (const std::filesystem::path &path);
  video (const std::filesystem::path &path, std::span<const std::byte> bytes,
         const metadata &meta);
  video (const std::filesystem::path &path, std::span<const pixel> pixels,
         const metadata &meta);

private:
  void encode_frame (const av_frame &frame);
  void flush_encoder ();

  av_codec_ctx codec_ctx_{};
  av_frame frame_{};
  av_format_ctx format_ctx_{};
  av_packet packet_{};

  std::filesystem::path path{};
  metadata metadata_{};
};

} // namespace ftv
