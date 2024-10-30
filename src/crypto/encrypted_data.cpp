#include "crypto/encrypted_data.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"

#include <fstream>
#include <utility>

namespace ftv
{

encrypted_data::encrypted_data (const std::filesystem::path &path)
{
  if (const auto data = read (path))
    {
      auto deserialized = deserialize_encrypted_data (*data);
      if (!deserialized)
        {
          throw std::runtime_error ("Failed to deserialize data");
        }
      ciphertext_ = std::move (deserialized->ciphertext ());
      init_vec_ = std::move (deserialized->init_vec ());
      tag_ = std::move (deserialized->tag ());
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

encrypted_data::encrypted_data (std::vector<std::byte> ciphertext,
                                std::vector<std::byte> init_vec,
                                std::vector<std::byte> tag)
    : ciphertext_ (std::move (ciphertext)), init_vec_ (std::move (init_vec)),
      tag_ (std::move (tag))
{
}

[[nodiscard]] std::expected<file, std::error_code>
encrypted_data::to_file (const std::filesystem::path &path) const noexcept
{
  const auto serialized = serialize_encrypted_data (*this);
  if (!serialized)
    {
      return std::expected<file, std::error_code>{ std::unexpected (
          serialized.error ()) };
    }
  return std::expected<file, std::error_code>{ file{ path, *serialized } };
}

[[nodiscard]] const std::vector<std::byte> &
encrypted_data::ciphertext () const noexcept
{
  return this->ciphertext_;
}

[[nodiscard]] const std::vector<std::byte> &
encrypted_data::init_vec () const noexcept
{
  return this->init_vec_;
}

[[nodiscard]] const std::vector<std::byte> &
encrypted_data::tag () const noexcept
{
  return this->tag_;
}

std::error_code
encrypted_data::save (const std::filesystem::path &path) const noexcept
{
  try
    {
      const auto serialized_data = serialize_encrypted_data (*this);

      std::ofstream file (path, std::ios::binary);
      if (!file)
        {
          return std::make_error_code (std::errc::no_such_file_or_directory);
        }

      file.write (reinterpret_cast<const char *> (serialized_data->data ()),
                  static_cast<std::streamsize> (serialized_data->size ()));

      if (!file)
        {
          return std::make_error_code (std::errc::io_error);
        }

      return {};
    }
  catch (...)
    {
      return std::make_error_code (std::errc::io_error);
    }
}

} // namespace ftv
