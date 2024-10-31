#pragma once

#include "crypto/encrypt.hpp"
#include "file/file.hpp"
#include "video/metadata.hpp"
#include "video/position.hpp"
#include "video/resolution.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

#include <opencv2/opencv.hpp>

namespace ftv
{

class video
{
public:
  video (const file &file, const secure_key &key,
         resolution res = resolution (1280, 720), std::size_t fps = 30);

  [[nodiscard]] std::size_t fps () const noexcept;
  [[nodiscard]] resolution res () const noexcept;
  [[nodiscard]] const metadata &meta_data () const noexcept;
  [[nodiscard]] const std::vector<std::byte> &data () const noexcept;
  [[nodiscard]] const std::vector<cv::Mat> &frames () const noexcept;
  [[nodiscard]] frame_position frame_pos () const noexcept;

  [[nodiscard]] std::error_code
  write (const std::filesystem::path &path) const noexcept;

private:
  void set_byte (std::byte byte) noexcept;

  void write_filename_size () noexcept;
  void write_filename () noexcept;
  void write_filesize () noexcept;
  void write_checksum () noexcept;
  void write_end_identifier () noexcept;
  void write_resolution () noexcept;
  void write_fps () noexcept;

  void init_metadata_frame () noexcept;

  [[nodiscard]] std::size_t calculate_required_frames () const noexcept;

  void advance_frame_position () noexcept;

  frame_position frame_pos_{};
  std::size_t fps_{};
  resolution res_{};
  metadata metadata_{};
  std::vector<std::byte> data_{};
  std::vector<cv::Mat> frames_{};
};

} // namespace ftv
