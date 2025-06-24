#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>
#include <vector>

namespace ftv
{

class file
{
public:
  explicit file (const std::filesystem::path &path);
  file (const std::filesystem::path &path, std::vector<std::byte> data);

  [[nodiscard]] const std::filesystem::path &path () const noexcept;
  [[nodiscard]] const std::vector<std::byte> &data () const noexcept;
  [[nodiscard]] std::size_t size () const noexcept;

private:
  std::filesystem::path path_;
  std::vector<std::byte> data_{};
};

[[nodiscard]] std::error_code write (const file &f,
                                     const std::filesystem::path &path);

[[nodiscard]] std::error_code write (std::span<const std::byte> data,
                                     const std::filesystem::path &path);

[[nodiscard]] std::error_code write (const file &f);

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
read (const std::filesystem::path &path) noexcept;

// payload layout, sizes are little-endian:
// [file name size (2 bytes)][file name][file contents]
// only the file name is stored, never the directories leading to it
[[nodiscard]] std::vector<std::byte> pack_file (const file &f);

// rejects names that are not a plain file name, e.g. "../x" or "/etc/x"
[[nodiscard]] std::expected<file, std::error_code>
unpack_file (std::span<const std::byte> payload) noexcept;

} // namespace ftv
