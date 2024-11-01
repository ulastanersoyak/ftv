#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"
#include "video/av_frame.hpp"
#include "video/pixel.hpp"

#include <print>

int
main ()
{
  ftv::file f{ "/home/retro/ftv/test/text.txt" };
  using namespace std::string_view_literals;
  ftv::secure_key key{ "1234567890" };

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

  ftv::av_frame frame{ { 1000, 1000 } };

  return 0;
}
