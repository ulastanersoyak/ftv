#include "crypto/serialize.hpp"
#include "video/pixel.hpp"

#include <print>

int
main ()
{
  // ftv::file f{ "/home/retro/ftv/test/text.txt" };
  // using namespace std::string_view_literals;
  // ftv::secure_key key{ "1234567890" };
  //
  // const auto encrypted = ftv::encrypt (f, key, ftv::aes_256_gcm);
  //
  // if (!encrypted)
  //   {
  //     return 1;
  //   }
  //
  // const auto serialized = ftv::serialize_encrypted_data (*encrypted);
  //
  // if (!serialized)
  //   {
  //     return 1;
  //   }

  std::vector<std::byte> vec{ std::byte (1), std::byte (2), std::byte (3),
                              std::byte (5), std::byte (6), std::byte (7) };
  const auto pix = ftv::bytes_to_pixels (vec);
  const auto byte = ftv::pixels_to_bytes (pix);

  return 0;
}
