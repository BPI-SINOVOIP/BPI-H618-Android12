#include <inttypes.h>

struct S
{
  int a;
  int b;
  int c;
};// end struct S

void
func(struct S *s)
{
  s->a = 1;
}
