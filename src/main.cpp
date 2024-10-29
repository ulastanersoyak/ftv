#include "file/file.hpp"

int
main ()
{
  ftv::file f{ "/home/retro/ftv/test/text.txt" };
  const auto a = ftv::write (f, "/home/retro/ftv/test/new_text.txt");
  if (a.value () != 0)
    {
      return -1;
    }
  return 0;
}
