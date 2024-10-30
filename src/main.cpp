#include "crypto/decrypt.hpp"
#include "crypto/encrypt.hpp"
#include "crypto/encrypted_data.hpp"
#include "crypto/serialize.hpp"
#include "file/file.hpp"

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

  const auto enc_file
      = encrypted->to_file ("/home/retro/ftv/test/text_encrypted");

  const auto err = ftv::write (*enc_file);

  ftv::encrypted_data enc_data{ enc_file->path () };
  const auto dec_file = ftv::decrypt (enc_data, key);

  const auto err1
      = ftv::write (*dec_file, "/home/retro/ftv/test/text_decrypted");

  return 0;
}
