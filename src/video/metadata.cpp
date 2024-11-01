#include "video/metadata.hpp"
#include <cstddef>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>

namespace ftv
{

metadata::metadata (std::span<const std::byte> bytes)
{
  std::size_t pos = 0;

  if (bytes.size () < sizeof (this->filename_size_))
    {
      throw std::runtime_error (
          std::format ("invalid metadata bytes: expected at least {} bytes "
                       "for filename_size",
                       sizeof (this->filename_size_)));
    }
  std::memcpy (&this->filename_size_, bytes.data () + pos,
               sizeof (this->filename_size_));
  pos += sizeof (this->filename_size_);

  if (bytes.size () < pos + this->filename_size_)
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for filename, got {}",
          this->filename_size_, bytes.size () - pos));
    }
  this->filename_.resize (this->filename_size_);
  std::memcpy (this->filename_.data (), bytes.data () + pos,
               this->filename_size_);
  pos += this->filename_size_;

  if (bytes.size () < pos + sizeof (this->file_size_))
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for file_size, got {}",
          sizeof (this->file_size_), bytes.size () - pos));
    }
  std::memcpy (&this->file_size_, bytes.data () + pos,
               sizeof (this->file_size_));
  pos += sizeof (this->file_size_);

  if (bytes.size () < pos + sizeof (this->checksum_))
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for checksum, got {}",
          sizeof (this->checksum_), bytes.size () - pos));
    }
  std::memcpy (&this->checksum_, bytes.data () + pos,
               sizeof (this->checksum_));
  pos += sizeof (this->checksum_);

  if (bytes.size () < pos + sizeof (this->fps_))
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for fps, got {}",
          sizeof (this->fps_), bytes.size () - pos));
    }
  std::memcpy (&this->fps_, bytes.data () + pos, sizeof (this->fps_));
  pos += sizeof (this->fps_);

  if (bytes.size () < pos + sizeof (this->res_))
    {
      throw std::runtime_error (
          std::format ("invalid metadata bytes: expected {} bytes for "
                       "resolution, got {}",
                       sizeof (this->res_), bytes.size () - pos));
    }
  std::memcpy (&this->res_, bytes.data () + pos, sizeof (this->res_));
}

metadata::metadata (std::string fname, std::size_t fsize, std::size_t checksum,
                    std::size_t fps, const resolution &r)
    : filename_size_ (fname.size ()), filename_ (std::move (fname)),
      file_size_ (fsize + this->size ()), checksum_ (checksum), fps_ (fps),
      res_ (r)
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
