#pragma once

#include "video/metadata.hpp"
#include "video/pixel.hpp"

#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <span>
#include <system_error>

namespace ftv
{

class video
{
public:
  explicit video (const std::filesystem::path &path);

  video (const std::filesystem::path &path, const metadata &meta);

  [[nodiscard]] std::error_code write (std::span<const std::byte> bytes) const;
  [[nodiscard]] std::error_code write (std::span<const pixel> pixels) const;

  void set_metadata (std::span<const std::byte> bytes);
  void set_metadata (const metadata &data);

private:
  void init_metadata ();

  [[nodiscard]] std::size_t read_size_t (const cv::Mat &frame,
                                         std::size_t start_pos) const;

  [[nodiscard]] std::string read_filename (const cv::Mat &frame,
                                           std::size_t start_pos,
                                           std::size_t filename_size) const;

  metadata metadata_;
  std::filesystem::path path_;
};

} // namespace ftv
