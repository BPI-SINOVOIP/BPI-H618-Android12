#include "test38.h"

int
main(int argc, char **argv)
{
  struct container_type c;
  bar(&c);
  baz(&c);
  return 0;
}
