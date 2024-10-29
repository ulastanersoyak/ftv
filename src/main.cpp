#include "crypto/encrypt.hpp"
#include "file/file.hpp"

int
main ()
{
  ftv::file f{ "/home/retro/ftv/test/text.txt" };
  const auto rs = ftv::encrypt (f, "123457890123", ftv::aes_256_gcm);
  return 0;
}
