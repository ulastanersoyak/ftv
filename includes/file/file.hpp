#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
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

[[nodiscard]] std::error_code write (const file &f);

[[nodiscard]] std::expected<std::vector<std::byte>, std::error_code>
read (const std::filesystem::path &path) noexcept;

} // namsepace ftv
