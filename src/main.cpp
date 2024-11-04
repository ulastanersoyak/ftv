#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "video/metadata.hpp"
#include "video/opencv/video.hpp"
#include "video/pixel.hpp"

#include <algorithm>
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
  auto ec = vid.write (*serialized);

  if (ec.value () != 0)
    {
      return 1;
    }

  ftv::video vid1{ "test.avi" };
  const auto pixels = vid1.read ();

  const auto bytes_from_vid = ftv::pixels_to_bytes (*pixels);

  if (!std::ranges::equal (bytes_from_vid, *serialized))
    {
      return 1;
    }

  const auto deserialized = ftv::deserialize_encrypted_data (bytes_from_vid);

  const auto file = ftv::decrypt (*deserialized, key);

  auto x = ftv::write (*file, vid1.get_metadata ().filename ().append ("x"));

  return 0;
}
