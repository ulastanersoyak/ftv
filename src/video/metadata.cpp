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

  if (bytes.size () < sizeof (filename_size))
    {
      throw std::runtime_error (
          std::format ("invalid metadata bytes: expected at least {} bytes "
                       "for filename_size",
                       sizeof (filename_size)));
    }
  std::memcpy (&filename_size, bytes.data () + pos, sizeof (filename_size));
  pos += sizeof (filename_size);

  if (bytes.size () < pos + filename_size)
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for filename, got {}",
          filename_size, bytes.size () - pos));
    }
  filename.resize (filename_size);
  std::memcpy (filename.data (), bytes.data () + pos, filename_size);
  pos += filename_size;

  if (bytes.size () < pos + sizeof (file_size))
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for file_size, got {}",
          sizeof (file_size), bytes.size () - pos));
    }
  std::memcpy (&file_size, bytes.data () + pos, sizeof (file_size));
  pos += sizeof (file_size);

  if (bytes.size () < pos + sizeof (checksum))
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for checksum, got {}",
          sizeof (checksum), bytes.size () - pos));
    }
  std::memcpy (&checksum, bytes.data () + pos, sizeof (checksum));
  pos += sizeof (checksum);

  if (bytes.size () < pos + sizeof (fps))
    {
      throw std::runtime_error (std::format (
          "invalid metadata bytes: expected {} bytes for fps, got {}",
          sizeof (fps), bytes.size () - pos));
    }
  std::memcpy (&fps, bytes.data () + pos, sizeof (fps));
  pos += sizeof (fps);

  if (bytes.size () < pos + sizeof (res))
    {
      throw std::runtime_error (
          std::format ("invalid metadata bytes: expected {} bytes for "
                       "resolution, got {}",
                       sizeof (res), bytes.size () - pos));
    }
  std::memcpy (&res, bytes.data () + pos, sizeof (res));
}

metadata::metadata (std::string fname, std::size_t fsize, std::size_t sum,
                    std::size_t frames_per_sec, const resolution &r)
    : filename_size (fname.size ()), filename (std::move (fname)),
      file_size (fsize), checksum (sum), fps (frames_per_sec), res (r)
{
  if (filename.empty ())
    {
      throw std::runtime_error ("filename cannot be empty");
    }
  if (fps == 0)
    {
      throw std::runtime_error (std::format ("invalid fps value: {}", fps));
    }
  if (res.x == 0 || res.y == 0)
    {
      throw std::runtime_error (
          std::format ("invalid resolution: {}x{}", res.x, res.y));
    }
}

[[nodiscard]] std::vector<std::byte>
metadata::to_vec () const noexcept
{
  std::vector<std::byte> bytes (this->size ());

  std::size_t pos = 0;

  const std::size_t fname_size = this->filename.size ();
  std::memcpy (bytes.data () + pos, &fname_size, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, this->filename.data (), filename.size ());
  pos += filename.size ();

  std::memcpy (bytes.data () + pos, &this->file_size, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, &this->checksum, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, &this->fps, sizeof (std::size_t));
  pos += sizeof (std::size_t);

  std::memcpy (bytes.data () + pos, &this->res, sizeof (resolution));

  return bytes;
}

} // namespace ftv
