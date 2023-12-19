#include "test38.h"

struct stub_type
{
  unsigned int m0;
  char m2;
  long m3;
};

long long
baz(struct container_type* c)
{
  return (long long) c;
}
