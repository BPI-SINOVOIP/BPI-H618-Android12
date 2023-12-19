#include <inttypes.h>

struct S
{
  struct
  {
    union
    {
      int a;
      char a_1;
    };
    union
    {
      int b;
      char b_1;
    };
  };
  int c;
}; // end struct S

void
func(struct S *s)
{
  s->a = 1;
}
