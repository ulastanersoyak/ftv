#include "video/metadata.hpp"
#include "video/pixel.hpp"
#include <cstddef>
#include <cstring>
#include <format>
#include <opencv2/videoio.hpp>
#include <stdexcept>
#include <vector>

namespace ftv
{

metadata::metadata (std::span<const std::byte> bytes)
{
  // check if there is  enough bytes to at least read the filename_size
  if (bytes.size () < sizeof (std::size_t))
    {
      throw std::runtime_error (
          std::format ("invalid metadata bytes: expected at least {} bytes "
                       "for filename_size",
                       sizeof (std::size_t)));
    }

  std::size_t pos = 0;

  // read filename_size first
  std::memcpy (&this->filename_size_, bytes.data () + pos,
               sizeof (std::size_t));
  pos += sizeof (std::size_t);

  const std::size_t required_size = sizeof (std::size_t) + // filename_size
                                    this->filename_size_ + // filename
                                    sizeof (std::size_t) + // file_size
                                    sizeof (std::size_t) + // checksum
                                    sizeof (std::size_t) + // fps
                                    sizeof (resolution);   // resolution

  // check if there is  enough bytes for everything
  if (bytes.size () < required_size)
    {
      throw std::runtime_error (
          std::format ("invalid metadata bytes: expected {} bytes, got {}",
                       required_size, bytes.size ()));
    }

  this->filename_.resize (this->filename_size_);
  std::memcpy (this->filename_.data (), bytes.data () + pos,
               this->filename_size_);
  pos += this->filename_size_;

  std::memcpy (&this->file_size_, bytes.data () + pos, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (&this->checksum_, bytes.data () + pos, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (&this->fps_, bytes.data () + pos, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (&this->res_, bytes.data () + pos, sizeof (resolution));
}

metadata::metadata (std::string fname, std::size_t fsize, std::size_t checksum,
                    std::size_t fps, const resolution &r)
    : filename_size_ (fname.size ()), filename_ (std::move (fname)),
      file_size_ (fsize), checksum_ (checksum), fps_ (fps), res_ (r)
{
  if (this->filename_.empty ())
    {
      throw std::runtime_error ("filename cannot be empty");
    }
  if (this->fps_ == 0)
    {
      throw std::runtime_error (std::format ("invalid fps value: {}", fps_));
    }
  if (this->res_.x == 0 || this->res_.y == 0)
    {
      throw std::runtime_error (std::format ("invalid resolution: {}x{}",
                                             this->res_.x, this->res_.y));
    }
}

[[nodiscard]] std::size_t
metadata::filename_size () const noexcept
{
  return this->filename_size_;
}

[[nodiscard]] std::string
metadata::filename () const noexcept
{
  return this->filename_;
}

[[nodiscard]] std::size_t
metadata::file_size () const noexcept
{
  return this->file_size_;
}

[[nodiscard]] std::size_t
metadata::checksum () const noexcept
{
  return this->checksum_;
}

[[nodiscard]] std::size_t
metadata::fps () const noexcept
{
  return this->fps_;
}

[[nodiscard]] resolution
metadata::res () const noexcept
{
  return this->res_;
}

[[nodiscard]] std::vector<std::byte>
metadata::to_vec () const noexcept
{
  std::vector<std::byte> bytes (this->size ());

  std::size_t pos = 0;

  const std::size_t fname_size = this->filename_.size ();
  std::memcpy (bytes.data () + pos, &fname_size, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, this->filename_.data (),
               this->filename_.size ());
  pos += this->filename_.size ();

  std::memcpy (bytes.data () + pos, &this->file_size_, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, &this->checksum_, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, &this->fps_, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, &this->res_, sizeof (resolution));

  return bytes;
}

} // namespace ftv
