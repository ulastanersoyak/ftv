#include "video/video.hpp"

namespace ftv
{

void
video::write_filename_size () noexcept
{
  const std::size_t filename_size = this->metadata_.filename.size ();
  for (std::size_t i = 0; i < sizeof (filename_size); ++i)
    {
      std::byte byte = std::byte (
          (filename_size >> (8 * (sizeof (filename_size) - 1 - i))) & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::write_filename () noexcept
{
  for (std::size_t i = 0; i < this->metadata_.filename.size (); ++i)
    {
      std::byte byte = std::byte (this->metadata_.filename[i]);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::write_filesize () noexcept
{
  for (std::size_t i = 0; i < sizeof (this->metadata_.filesize); ++i)
    {
      std::byte byte
          = std::byte ((this->metadata_.filesize
                        >> (8 * (sizeof (this->metadata_.filesize) - 1 - i)))
                       & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::write_checksum () noexcept
{
  for (std::size_t i = 0; i < sizeof (this->metadata_.checksum); ++i)
    {
      std::byte byte
          = std::byte ((this->metadata_.checksum
                        >> (8 * (sizeof (this->metadata_.checksum) - 1 - i)))
                       & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::write_end_identifier () noexcept
{
  const uint32_t end_identifier = 0xDEADBEEF;
  for (std::size_t i = 0; i < sizeof (end_identifier); ++i)
    {
      std::byte byte = std::byte (
          (end_identifier >> (8 * (sizeof (end_identifier) - 1 - i))) & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::write_fps () noexcept
{
  for (std::size_t i = 0; i < sizeof (this->metadata_.fps); ++i)
    {
      std::byte byte = std::byte (
          (this->metadata_.fps >> (8 * (sizeof (this->metadata_.fps) - 1 - i)))
          & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::write_resolution () noexcept
{
  // write width
  for (std::size_t i = 0; i < sizeof (this->metadata_.width); ++i)
    {
      std::byte byte
          = std::byte ((this->metadata_.width
                        >> (8 * (sizeof (this->metadata_.width) - 1 - i)))
                       & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }

  // write height
  for (std::size_t i = 0; i < sizeof (this->metadata_.height); ++i)
    {
      std::byte byte
          = std::byte ((this->metadata_.height
                        >> (8 * (sizeof (this->metadata_.height) - 1 - i)))
                       & 0xFF);
      this->set_byte (byte);
      this->advance_frame_position ();
    }
}

void
video::init_metadata_frame () noexcept
{
  // each byte takes 2 pixels (4 bits per pixel in RGBA channels)
  // a single byte takes 2 bytes to store on frame
  // metadata layout (first row):
  // [0   -   15]: filename size (8 bytes = 16 pixels)
  // [16  -    X]: filename (filename_size * 2 pixels)
  // [X+1 - X+16]: filesize (8 bytes = 16 pixels)
  // [X+17- X+32]: checksum (8 bytes = 16 pixels)
  // [X+33- X+48]: fps (8 bytes = 16 pixels)
  // [X+49- X+64]: resolution width (8 bytes = 16 pixels)
  // [X+65- X+80]: resolution height (8 bytes = 16 pixels)
  // [X+81- X+88]: end identifier 0xDEADBEEF (4 bytes = 8 pixels)
  this->frame_pos_ = frame_position{};

  write_filename_size ();
  write_filename ();
  write_filesize ();
  write_checksum ();
  write_fps ();
  write_resolution ();
  write_end_identifier ();
}

} // namespace ftv
