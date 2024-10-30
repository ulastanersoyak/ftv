#pragma once

#include "file/file.hpp"

#include <cstddef>
#include <filesystem>
#include <system_error>
#include <vector>

namespace ftv
{

class encrypted_data
{
public:
  explicit encrypted_data (const std::filesystem::path &path);

  encrypted_data (std::vector<std::byte> ciphertext,
                  std::vector<std::byte> init_vec, std::vector<std::byte> tag);

  encrypted_data (std::vector<std::byte> &&ciphertext,
                  std::vector<std::byte> &&init_vec,
                  std::vector<std::byte> &&tag);

  [[nodiscard]] std::expected<file, std::error_code>
  to_file (const std::filesystem::path &path
           = std::filesystem::temp_directory_path ()
             / "encrypted_file") const noexcept;

  [[nodiscard]] const std::vector<std::byte> &ciphertext () const noexcept;
  [[nodiscard]] const std::vector<std::byte> &init_vec () const noexcept;
  [[nodiscard]] const std::vector<std::byte> &tag () const noexcept;

  std::error_code save (const std::filesystem::path &path) const noexcept;

private:
  std::vector<std::byte> ciphertext_{};
  std::vector<std::byte> init_vec_{}; // generated during encryption
  std::vector<std::byte> tag_{};      // generated during encryption
};

} // namespace ftv
