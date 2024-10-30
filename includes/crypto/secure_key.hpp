#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace ftv
{

class secure_key
{

public:
  explicit secure_key (std::string_view key);

  ~secure_key ();

  [[nodiscard]] std::span<const std::byte> get () const noexcept;

  [[nodiscard]] std::size_t size () const noexcept;

private:
  std::vector<std::byte> key_{};
};

} // namespace ftv
