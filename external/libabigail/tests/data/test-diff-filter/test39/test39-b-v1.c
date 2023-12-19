#include "test39.h"

struct opaque_type
{
  int m0;
  char m1;
};

long long
foo1(struct container_type* c)
{
  return (long long) c;
}
