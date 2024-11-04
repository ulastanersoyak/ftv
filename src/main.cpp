#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "video/metadata.hpp"
#include "video/opencv/video.hpp"

#include <print>
#include <string_view>

int
main ()
{
  using namespace std::string_view_literals;
  std::string fname{ "/home/retro/ftv/test/text.txt"sv };
  ftv::file f{ fname };
  ftv::secure_key key{ "1234567890"sv };

  const auto encrypted = ftv::encrypt (f, key, ftv::aes_256_gcm);

  if (!encrypted)
    {
      return 1;
    }

  const auto serialized = ftv::serialize_encrypted_data (*encrypted);

  if (!serialized)
    {
      return 1;
    }

  ftv::metadata data{ fname, serialized->size (), 100, 5, { 300, 300 } };
  ftv::video vid{ "test.avi", data };
  vid.write (*serialized);

  ftv::video vid1{ "test.avi" };

  return 0;
}
