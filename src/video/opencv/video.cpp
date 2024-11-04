#include <opencv2/opencv.hpp>
#include <video/opencv/video.hpp>

#include "video/opencv/video_io.hpp"
#include "video/pixel.hpp"

namespace ftv
{

video::video (const std::filesystem::path &path, const metadata &meta)
    : metadata_{ meta }, path_{ path }
{
}

video::video (const std::filesystem::path &path) : metadata_{}, path_ (path)
{
  init_metadata ();
}

[[nodiscard]] std::error_code
video::write (std::span<const pixel> pixels) const
{
  if (this->metadata_.size () == 0)
    {
      return std::make_error_code (std::errc::invalid_argument);
    }

  const std::size_t pixels_per_frame
      = this->metadata_.res ().x * this->metadata_.res ().y;

  if (pixels.empty ())
    {
      return {};
    }

  video_writer writer{ path_ };
  writer.get ().set (cv::VIDEOWRITER_PROP_QUALITY, 100);

  writer.get ().open (
      this->path_.string (), cv::VideoWriter::fourcc ('M', 'J', 'P', 'G'),
      static_cast<double> (this->metadata_.fps ()),
      cv::Size (static_cast<std::int32_t> (this->metadata_.res ().x),
                static_cast<std::int32_t> (this->metadata_.res ().y)));
  if (!writer.get ().isOpened ())
    {
      return std::make_error_code (std::errc::io_error);
    }
  for (size_t offset = 0; offset < pixels.size (); offset += pixels_per_frame)
    {
      cv::Mat frame (static_cast<std::int32_t> (this->metadata_.res ().y),
                     static_cast<std::int32_t> (this->metadata_.res ().x),
                     CV_8UC3);

      const size_t pixels_to_copy
          = std::min (pixels_per_frame, pixels.size () - offset);

      const std::int32_t last_row = static_cast<std::int32_t> (
          pixels_to_copy / static_cast<std::size_t> (frame.cols));

      const std::int32_t last_col = static_cast<std::int32_t> (
          pixels_to_copy % static_cast<std::size_t> (frame.cols));

      size_t pixel_idx = 0;
      for (int y = 0; y < last_row; ++y)
        {
          for (int x = 0; x < frame.cols; ++x)
            {
              const auto &pix = pixels[offset + pixel_idx++];
              uint8_t pixel_val = (pix.b != 0) ? 255 : 0; // Black or white
              frame.at<cv::Vec3b> (y, x)
                  = cv::Vec3b (pixel_val, pixel_val, pixel_val);
            }
        }
      // fill partial row if needed
      for (int x = 0; x < last_col; ++x)
        {
          const auto &pix = pixels[offset + pixel_idx++];
          uint8_t pixel_val = (pix.b != 0) ? 255 : 0;
          frame.at<cv::Vec3b> (last_row, x)
              = cv::Vec3b (pixel_val, pixel_val, pixel_val);
        }
      // fill remaining pixels with black (if needed)
      if (pixels_to_copy < pixels_per_frame)
        {
          // fill rest of the last row
          for (int x = last_col; x < frame.cols; ++x)
            {
              frame.at<cv::Vec3b> (last_row, x) = cv::Vec3b (0, 0, 0);
            }
          // fill remaining rows
          for (int y = last_row + 1; y < frame.rows; ++y)
            {
              for (int x = 0; x < frame.cols; ++x)
                {
                  frame.at<cv::Vec3b> (y, x) = cv::Vec3b (0, 0, 0);
                }
            }
        }
      writer.get ().write (frame);
    }
  return {};
}

[[nodiscard]] std::error_code
video::write (std::span<const std::byte> bytes) const
{
  const auto metadata_vec = this->metadata_.to_vec ();
  const auto metadata_pixels = bytes_to_pixels (metadata_vec);

  std::vector<pixel> all_pixels;
  all_pixels.reserve (metadata_pixels.size () + bytes.size ());
  all_pixels.insert (all_pixels.end (), metadata_pixels.begin (),
                     metadata_pixels.end ());

  const auto data_pixels = bytes_to_pixels (bytes);
  all_pixels.insert (all_pixels.end (), data_pixels.begin (),
                     data_pixels.end ());

  return write (all_pixels);
}

void
video::init_metadata ()
{
  video_reader reader{ path_ };
  cv::Mat frame;
  if (!reader.get ().read (frame) || frame.empty ()
      || frame.type () != CV_8UC3)
    {
      throw std::runtime_error ("failed to read first frame");
    }

  std::size_t pos = 0;

  std::size_t filename_size = read_size_t (frame, pos);
  pos += sizeof (std::size_t) * 8;

  std::string filename = read_filename (frame, pos, filename_size);
  pos += filename_size * 8;

  std::size_t file_size = read_size_t (frame, pos);
  pos += sizeof (std::size_t) * 8;

  std::size_t checksum = read_size_t (frame, pos);
  pos += sizeof (std::size_t) * 8;

  std::size_t fps = read_size_t (frame, pos);
  pos += sizeof (std::size_t) * 8;

  auto x = read_size_t (frame, pos);
  pos += sizeof (std::size_t) * 8;
  auto y = read_size_t (frame, pos);
  resolution res{ x, y };

  metadata_ = metadata (filename, file_size, checksum, fps, res);
}

[[nodiscard]] std::size_t
video::read_size_t (const cv::Mat &frame, std::size_t start_pos) const
{
  std::vector<std::byte> size_bytes (sizeof (std::size_t));
  for (std::size_t i = 0; i < sizeof (std::size_t) * 8; i += 8)
    {
      uint8_t byte_val = 0;
      for (size_t bit = 0; bit < 8; ++bit)
        {
          cv::Vec3b bgr_px = frame.at<cv::Vec3b> (
              0, static_cast<std::int32_t> (start_pos + i + bit));
          bool is_white
              = (bgr_px[0] > 127) || (bgr_px[1] > 127) || (bgr_px[2] > 127);
          if (is_white)
            {
              byte_val |= (1 << (7 - bit));
            }
        }
      size_bytes.at (i / 8) = std::byte (byte_val);
    }
  std::size_t result = 0;
  std::memcpy (&result, size_bytes.data (), sizeof (std::size_t));
  return result;
}

[[nodiscard]] std::string
video::read_filename (const cv::Mat &frame, std::size_t start_pos,
                      std::size_t filename_size) const
{
  std::vector<std::byte> filename_bytes;
  filename_bytes.reserve (filename_size);

  for (std::size_t i = 0; i < filename_size * 8; i += 8)
    {
      uint8_t byte_val = 0;
      for (size_t bit = 0; bit < 8; ++bit)
        {
          cv::Vec3b bgr_px = frame.at<cv::Vec3b> (
              0, static_cast<std::int32_t> (start_pos + i + bit));
          bool is_white
              = (bgr_px[0] > 127) || (bgr_px[1] > 127) || (bgr_px[2] > 127);
          if (is_white)
            {
              byte_val |= (1 << (7 - bit));
            }
        }
      filename_bytes.push_back (std::byte (byte_val));
    }

  return std::string (reinterpret_cast<const char *> (filename_bytes.data ()),
                      filename_bytes.size ());
}

} // namespace ftv