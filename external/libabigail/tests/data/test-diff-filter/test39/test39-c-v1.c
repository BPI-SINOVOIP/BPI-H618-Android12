#include "test39.h"

struct opaque_type
{
  unsigned m0;
  unsigned char m1;
};

long long
foo2(struct container_type* c)
{
  return (long long) c;
}
