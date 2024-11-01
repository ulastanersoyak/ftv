#pragma once

#include "video/av_codec_ctx.hpp"
#include "video/av_format_ctx.hpp"
#include "video/av_frame.hpp"
#include "video/av_packet.hpp"
#include "video/metadata.hpp"
#include "video/pixel.hpp"

#include <cstddef>
#include <filesystem>
#include <span>

namespace ftv
{

class media
{
public:
  explicit media (const std::filesystem::path &path);
  media (std::span<const std::byte> bytes, const metadata &meta);
  media (std::span<const pixel> pixels, const metadata &meta);

  virtual ~media () = default;

  [[nodiscard]] virtual const metadata &get_metadata () const noexcept;
  virtual void write_to_file (const std::filesystem::path &path) const;
  [[nodiscard]] virtual std::vector<std::byte> to_bytes () const;
  [[nodiscard]] virtual std::vector<pixel> to_pixels () const;

protected:
  void init_codec (bool is_video);
  void setup_format_context (bool is_video);
  virtual void process_frame () = 0;

  av_codec_ctx codec_ctx_;
  av_frame frame_;
  av_format_ctx format_ctx_;
  av_packet packet_;
  metadata metadata_{};

private:
  void initialize_from_path (const std::filesystem::path &path);
  void initialize_from_bytes (std::span<const std::byte> bytes);
  void initialize_from_pixels (std::span<const pixel> pixels);
};

} // namespace ftv
