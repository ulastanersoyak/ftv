#include "file/file.hpp"
#include "util/endian.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace ftv
{

file::file (const std::filesystem::path &path) : path_{ path }
{
  if (const auto data = read (path))
    {
      this->data_ = data.value ();
    }
  else
    {
      const auto &error = data.error ();
      switch (error.value ())
        {
        case static_cast<std::int32_t> (std::errc::no_such_file_or_directory):
          throw std::runtime_error (
              std::format ("file does not exist: {}", path.c_str ()));

        case static_cast<std::int32_t> (std::errc::file_too_large):
          throw std::runtime_error (
              std::format ("file is too large: {}", path.c_str ()));

        case static_cast<std::int32_t> (std::errc::io_error):
          throw std::runtime_error (
              std::format ("failed to read file: {}", path.c_str ()));

        default:
          throw std::runtime_error (
              std::format ("unknown error reading file {}: {}", path.c_str (),
                           error.message ()));
        }
    }
}

file::file (const std::filesystem::path &path, std::vector<std::byte> data)
    : path_ (path), data_ (std::move (data))
{
}

[[nodiscard]] const std::filesystem::path &
file::path () const noexcept
{
  return this->path_;
}

[[nodiscard]] const std::vector<std::byte> &
file::data () const noexcept
{
  return this->data_;
}

[[nodiscard]] std::size_t
file::size () const noexcept
{
  return this->data_.size ();
}

[[nodiscard]] std::error_code
write (std::span<const std::byte> data, const std::filesystem::path &path)
{
  if (std::filesystem::exists (path))
    {
      return std::make_error_code (std::errc::file_exists);
    }
  if (path.empty ())
    {
      return make_error_code (std::errc::invalid_argument);
    }
  std::ofstream file_stream (path, std::ios::binary);

  if (!file_stream.is_open ())
    {
      return std::make_error_code (std::errc::no_such_file_or_directory);
    }

  file_stream.write (reinterpret_cast<const char *> (data.data ()),
                     static_cast<std::streamsize> (data.size ()));

  if (!file_stream)
    {
      return std::make_error_code (std::errc::io_error);
    }

  return {};
}

[[nodiscard]] std::error_code
write (const file &f, const std::filesystem::path &path)
{
  return write (f.data (), path);
}

[[nodiscard]] std::error_code
write (const file &f)
{
  return write (f, f.path ());
}

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
read (const std::filesystem::path &path) noexcept
{
  if (!std::filesystem::exists (path))
    {
      return std::expected<std::vector<std::byte>, std::error_code> (
          std::unexpected (
              std::make_error_code (std::errc::no_such_file_or_directory)));
    }

  std::ifstream file_stream (path, std::ios::binary);
  if (!file_stream.is_open ())
    {
      return std::expected<std::vector<std::byte>, std::error_code> (
          std::unexpected (std::make_error_code (std::errc::io_error)));
    }

  auto file_size = std::filesystem::file_size (path);

  if (file_size > std::numeric_limits<std::size_t>::max ())
    {
      return std::expected<std::vector<std::byte>, std::error_code> (
          std::unexpected (std::make_error_code (std::errc::file_too_large)));
    }

  std::vector<std::byte> data (file_size);
  file_stream.read (reinterpret_cast<char *> (data.data ()),
                    static_cast<std::streamsize> (file_size));

  if (!file_stream)
    {
      return std::expected<std::vector<std::byte>, std::error_code> (
          std::unexpected (std::make_error_code (std::errc::io_error)));
    }

  return std::expected<std::vector<std::byte>, std::error_code> (
      std::move (data));
}

[[nodiscard]] std::vector<std::byte>
pack_file (const file &f)
{
  const auto name = f.path ().filename ().string ();
  if (name.empty ()
      || name.size () > std::numeric_limits<std::uint16_t>::max ())
    {
      throw std::invalid_argument (
          std::format ("invalid file name: {}", f.path ().string ()));
    }

  std::vector<std::byte> payload{};
  payload.reserve (sizeof (std::uint16_t) + name.size () + f.size ());
  put_le (payload, static_cast<std::uint16_t> (name.size ()));
  std::ranges::transform (name, std::back_inserter (payload),
                          [] (char c) { return static_cast<std::byte> (c); });
  payload.insert (payload.end (), f.data ().begin (), f.data ().end ());
  return payload;
}

[[nodiscard]] std::expected<file, std::error_code>
unpack_file (std::span<const std::byte> payload) noexcept
{
  try
    {
      if (payload.size () < sizeof (std::uint16_t))
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }
      const std::size_t name_size = get_le<std::uint16_t> (payload, 0);
      if (payload.size () - sizeof (std::uint16_t) < name_size)
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }

      std::string name (name_size, '\0');
      std::ranges::transform (
          payload.subspan (sizeof (std::uint16_t), name_size), name.begin (),
          [] (std::byte b) { return std::to_integer<char> (b); });

      // the name is used as an output path, so only accept a plain file
      // name that cannot point outside the current directory
      const std::filesystem::path name_path{ name };
      if (name.empty () || name == "." || name == ".."
          || name.find ('\0') != std::string::npos
          || name_path.filename () != name_path)
        {
          return std::unexpected (
              std::make_error_code (std::errc::invalid_argument));
        }

      const auto contents
          = payload.subspan (sizeof (std::uint16_t) + name_size);
      return file{ name_path, std::vector<std::byte> (contents.begin (),
                                                      contents.end ()) };
    }
  catch (...)
    {
      return std::unexpected (
          std::make_error_code (std::errc::not_enough_memory));
    }
}

} // namespace ftv
