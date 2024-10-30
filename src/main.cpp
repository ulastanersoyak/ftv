#include "crypto/encrypt.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"
#include <algorithm>

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

  const auto deserialized = ftv::deserialize_encrypted_data (*serialized);
  if (!deserialized)
    {
      return 1;
    }

  bool are_equal = encrypted->ciphertext == deserialized->ciphertext
                   && encrypted->init_vec == deserialized->init_vec
                   && encrypted->tag == deserialized->tag;

  const auto reserialized = ftv::serialize_encrypted_data (*deserialized);
  bool serialized_equal = std::ranges::equal (*serialized, *reserialized);

  return are_equal && serialized_equal ? 0 : 1;
}
