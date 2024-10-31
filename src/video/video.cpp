#include "video/video.hpp"
#include "crypto/checksum.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"
#include "opencv2/core/core.hpp"
#include "video/video_error.hpp"

#include <filesystem>
#include <string>
#include <system_error>

namespace ftv
{

video::video (const file &file, const secure_key &key, resolution res,
              std::size_t fps)
    : fps_{ fps }, res_{ res }
{
  if (res.x == 0 || res.y == 0 || res.x > MAX_RESOLUTION
      || res.y > MAX_RESOLUTION)
    {
      throw video_error ("invalid resolution",
                         video_error_code::invalid_resolution);
    }

  if (fps == 0 || fps > MAX_FPS)
    {
      throw video_error ("invalid fps", video_error_code::invalid_fps);
    }

  const auto encrypted_file = encrypt (file, key);
  if (!encrypted_file)
    {
      throw video_error ("failed to encrypt file",
                         video_error_code::encryption_failed);
    }

  const auto serialized_data = serialize_encrypted_data (*encrypted_file);
  if (!serialized_data)
    {
      throw video_error ("failed to serialize file",
                         video_error_code::serialization_failed);
    }
  this->data_ = *serialized_data;

  this->metadata_.filesize = this->data_.size ();
  this->metadata_.filename = file.path ().filename ().string ();
  this->metadata_.checksum = hash (this->data_);
  this->metadata_.fps = fps;
  this->metadata_.width = res.x;
  this->metadata_.height = res.y;

  const std::size_t required_frames = calculate_required_frames ();
  this->frames_.reserve (required_frames);
  // pre-initialize first frame
  frames_.emplace_back (static_cast<int32_t> (this->res_.y),
                        static_cast<int32_t> (this->res_.x), CV_8UC4);
  this->init_metadata_frame ();
}

[[nodiscard]] std::size_t
video::fps () const noexcept
{
  return this->fps_;
}

[[nodiscard]] resolution
video::res () const noexcept
{
  return this->res_;
}

[[nodiscard]] const metadata &
video::meta_data () const noexcept
{
  return this->metadata_;
}

[[nodiscard]] const std::vector<std::byte> &
video::data () const noexcept
{
  return this->data_;
}

[[nodiscard]] const std::vector<cv::Mat> &
video::frames () const noexcept
{
  return this->frames_;
}

[[nodiscard]] frame_position
video::frame_pos () const noexcept
{
  return this->frame_pos_;
}
[[nodiscard]] std::error_code
video::write (const std::filesystem::path &path) const noexcept
{
  try
    {
      auto video_path = path;
      if (video_path.extension () != ".mp4")
        {
          video_path += ".mp4";
        }

      if (std::filesystem::exists (video_path))
        {
          return std::make_error_code (std::errc::file_exists);
        }

      std::string pipeline
          = "appsrc ! videoconvert ! x264enc ! mp4mux ! filesink location="
            + video_path.string ();

      cv::VideoWriter video_writer (
          pipeline, 0, static_cast<double> (this->fps_),
          cv::Size (static_cast<std::int32_t> (this->res_.x),
                    static_cast<std::int32_t> (this->res_.y)),
          true);

      if (!video_writer.isOpened ())
        {
          return std::make_error_code (std::errc::io_error);
        }

      for (const auto &frame : this->frames_)
        {
          cv::Mat rgb_frame (frame.rows, frame.cols, CV_8UC3, frame.data);
          video_writer.write (rgb_frame);
        }

      video_writer.release ();
      return {};
    }
  catch (...)
    {
      return std::make_error_code (std::errc::io_error);
    }
}

void
video::set_byte (std::byte byte) noexcept
{
  // first pixel: high nibble (bits 7-4)
  auto &pixel1 = this->frames_.at (this->frame_pos_.frame_index)
                     .at<cv::Vec4b> (
                         static_cast<std::int32_t> (this->frame_pos_.pos.y),
                         static_cast<std::int32_t> (this->frame_pos_.pos.x));
  pixel1[0]
      = (std::to_integer<uint8_t> (byte) & 0b10000000) ? 255 : 0; // bit 7
  pixel1[1]
      = (std::to_integer<uint8_t> (byte) & 0b01000000) ? 255 : 0; // bit 6
  pixel1[2]
      = (std::to_integer<uint8_t> (byte) & 0b00100000) ? 255 : 0; // bit 5
  pixel1[3]
      = (std::to_integer<uint8_t> (byte) & 0b00010000) ? 255 : 0; // bit 4

  // second pixel: low nibble (bits 3-0)
  auto &pixel2
      = this->frames_.at (this->frame_pos_.frame_index)
            .at<cv::Vec4b> (
                static_cast<std::int32_t> (this->frame_pos_.pos.y),
                static_cast<std::int32_t> (this->frame_pos_.pos.x + 1));
  pixel2[0]
      = (std::to_integer<uint8_t> (byte) & 0b00001000) ? 255 : 0; // bit 3
  pixel2[1]
      = (std::to_integer<uint8_t> (byte) & 0b00000100) ? 255 : 0; // bit 2
  pixel2[2]
      = (std::to_integer<uint8_t> (byte) & 0b00000010) ? 255 : 0; // bit 1
  pixel2[3]
      = (std::to_integer<uint8_t> (byte) & 0b00000001) ? 255 : 0; // bit 0
}

[[nodiscard]] std::size_t
video::calculate_required_frames () const noexcept
{
  // each byte takes 2 pixels
  const std::size_t pixels_per_byte = 2;

  const std::size_t metadata_size
      = sizeof (std::size_t) +              // filename size
        this->metadata_.filename.size () +  // filename
        sizeof (this->metadata_.filesize) + // filesize
        sizeof (this->metadata_.checksum) + // checksum
        sizeof (std::uint32_t) +            // end identifier (0xDEADBEEF)
        sizeof (uint32_t);                  // end identifier (0xDEADBEEF)
  const std::size_t total_bytes = metadata_size + this->data_.size ();

  const std::size_t total_pixels_needed = total_bytes * pixels_per_byte;

  const std::size_t pixels_per_frame = this->res_.x * this->res_.y;

  return (total_pixels_needed + pixels_per_frame - 1) / pixels_per_frame;
}

void
video::advance_frame_position () noexcept
{
  std::size_t new_x
      = this->frame_pos_.pos.x + 2; // advance by 2 pixels (1 byte)
  std::size_t new_y = this->frame_pos_.pos.y + (new_x / this->res_.x);

  // if frame bounds are exceeded
  if (new_y >= this->res_.y)
    {
      // move to next frame
      this->frame_pos_.frame_index++;
      // reset to top-left corner
      this->frame_pos_.pos.x = 0;
      this->frame_pos_.pos.y = 0;
      // init next frame if it doesn't exist
      if (this->frame_pos_.frame_index >= this->frames_.size ())
        {
          this->frames_.emplace_back (static_cast<int32_t> (this->res_.y),
                                      static_cast<int32_t> (this->res_.x),
                                      CV_8UC4);
        }
    }
  else
    {
      this->frame_pos_.pos.y = new_y;
      this->frame_pos_.pos.x = new_x % this->res_.x;
    }
}

} // namespace ftv
