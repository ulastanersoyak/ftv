#include "video/video.hpp"
#include "crypto/checksum.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"
#include "video/video_error.hpp"

#include <filesystem>
#include <string>
#include <system_error>

namespace ftv
{

cv::Mat
video::test ()
{
  return this->metadata_frame ();
}

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
}

[[nodiscard]] std::error_code
video::write (const std::filesystem::path &path) const noexcept
{
  if (std::filesystem::exists (path))
    {
      return std::make_error_code (std::errc::file_exists);
    }

  return {};
}

void
video::set_byte (cv::Mat &frame, position pos, std::byte byte) const noexcept
{
  // first pixel: high nibble (bits 7-4)
  frame.at<cv::Vec4b> (static_cast<std::int32_t> (pos.y),
                       static_cast<std::int32_t> (pos.x))
      = cv::Vec4b (
          (std::to_integer<uint8_t> (byte) & 0b10000000) ? 255 : 0, // bit 7
          (std::to_integer<uint8_t> (byte) & 0b01000000) ? 255 : 0, // bit 6
          (std::to_integer<uint8_t> (byte) & 0b00100000) ? 255 : 0, // bit 5
          (std::to_integer<uint8_t> (byte) & 0b00010000) ? 255 : 0  // bit 4
      );

  // second pixel: low nibble (bits 3-0)
  frame.at<cv::Vec4b> (static_cast<std::int32_t> (pos.y),
                       static_cast<std::int32_t> (pos.x + 1))
      = cv::Vec4b (
          (std::to_integer<uint8_t> (byte) & 0b00001000) ? 255 : 0, // bit 3
          (std::to_integer<uint8_t> (byte) & 0b00000100) ? 255 : 0, // bit 2
          (std::to_integer<uint8_t> (byte) & 0b00000010) ? 255 : 0, // bit 1
          (std::to_integer<uint8_t> (byte) & 0b00000001) ? 255 : 0  // bit 0
      );
}

void
video::write_filename_size (cv::Mat &frame, size_t start_x) const noexcept
{
  const std::size_t filename_size = this->metadata_.filename.size ();
  for (std::size_t i = 0; i < sizeof (filename_size); ++i)
    {
      std::byte byte = std::byte (
          (filename_size >> (8 * (sizeof (filename_size) - 1 - i))) & 0xFF);
      this->set_byte (frame, { .x = start_x + (i * 2), .y = 0 }, byte);
    }
}

void
video::write_filename (cv::Mat &frame, size_t start_x) const noexcept
{
  for (std::size_t i = 0; i < this->metadata_.filename.size (); ++i)
    {
      std::byte byte = std::byte (this->metadata_.filename[i]);
      this->set_byte (frame, { .x = start_x + (i * 2), .y = 0 }, byte);
    }
}

void
video::write_filesize (cv::Mat &frame, size_t start_x) const noexcept
{
  for (std::size_t i = 0; i < sizeof (this->metadata_.filesize); ++i)
    {
      std::byte byte
          = std::byte ((this->metadata_.filesize
                        >> (8 * (sizeof (this->metadata_.filesize) - 1 - i)))
                       & 0xFF);
      this->set_byte (frame, { .x = start_x + (i * 2), .y = 0 }, byte);
    }
}

void
video::write_checksum (cv::Mat &frame, size_t start_x) const noexcept
{
  for (std::size_t i = 0; i < sizeof (this->metadata_.checksum); ++i)
    {
      std::byte byte
          = std::byte ((this->metadata_.checksum
                        >> (8 * (sizeof (this->metadata_.checksum) - 1 - i)))
                       & 0xFF);
      this->set_byte (frame, { .x = start_x + (i * 2), .y = 0 }, byte);
    }
}

void
video::write_end_identifier (cv::Mat &frame, size_t start_x) const noexcept
{
  const uint32_t end_identifier = 0xDEADBEEF;
  for (std::size_t i = 0; i < sizeof (end_identifier); ++i)
    {
      std::byte byte = std::byte (
          (end_identifier >> (8 * (sizeof (end_identifier) - 1 - i))) & 0xFF);
      this->set_byte (frame, { .x = start_x + (i * 2), .y = 0 }, byte);
    }
}

[[nodiscard]] cv::Mat
video::metadata_frame () const noexcept
{
  cv::Mat data_frame (static_cast<int32_t> (this->res_.y),
                      static_cast<int32_t> (this->res_.x), CV_8UC4);

  data_frame = cv::Scalar (0, 0, 0, 0);

  // each byte takes 2 pixels (4 bits per pixel in RGBA channels)
  // a single byte takes 2 bytes to store on frame
  // metadata layout (first row):
  // [0   -   15]: filename size (8 bytes = 16 pixels)
  // [16  -    X]: filename (filename_size * 2 pixels)
  // [X+1 - X+16]: filesize (8 bytes = 16 pixels)
  // [X+17- X+32]: checksum (8 bytes = 16 pixels)
  // [X+33- X+40]: end identifier 0xDEADBEEF (4 bytes = 8 pixels)

  std::size_t current_x = 0;
  write_filename_size (data_frame, current_x);

  current_x = sizeof (std::size_t) * 2;
  write_filename (data_frame, current_x);

  current_x += this->metadata_.filename.size () * 2;
  write_filesize (data_frame, current_x);

  current_x += sizeof (this->metadata_.filesize) * 2;
  write_checksum (data_frame, current_x);

  current_x += sizeof (this->metadata_.checksum) * 2;
  write_end_identifier (data_frame, current_x);

  return data_frame;
}

} // namespace ftv
