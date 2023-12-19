#include <inttypes.h>

struct S
{
  uint64_t marker[0];
  uint64_t a;
  uint64_t b;
  uint64_t c;
};

void
func(struct S *s)
{
  s->a = 1;
}
