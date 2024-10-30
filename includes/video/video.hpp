#pragma once

#include "crypto/encrypt.hpp"
#include "file/file.hpp"
#include "video/metadata.hpp"
#include "video/position.hpp"
#include "video/resolution.hpp"

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

  [[nodiscard]] std::error_code
  write (const std::filesystem::path &path) const noexcept;
  cv::Mat test ();

private:
  void set_byte (cv::Mat &frame, position pos, std::byte byte) const noexcept;

  void write_filename_size (cv::Mat &frame, size_t start_x) const noexcept;
  void write_filename (cv::Mat &frame, size_t start_x) const noexcept;
  void write_filesize (cv::Mat &frame, size_t start_x) const noexcept;
  void write_checksum (cv::Mat &frame, size_t start_x) const noexcept;
  void write_end_identifier (cv::Mat &frame, size_t start_x) const noexcept;
  [[nodiscard]] cv::Mat metadata_frame () const noexcept;

  std::size_t fps_{};
  resolution res_{};
  metadata metadata_{};
  std::vector<std::byte> data_{};
};

} // namespace ftv
