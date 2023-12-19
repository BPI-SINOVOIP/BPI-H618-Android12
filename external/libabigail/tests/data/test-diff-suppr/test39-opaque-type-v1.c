#include "test39-public-headers-dir/test39-header-v1.h"

struct _StructType
{
  int m0;
  char m1;
};

struct _AnotherStructType
{
  int m1;
};


void
foo(StructType* a __attribute__((unused)))
{
}

void bar(AnotherStructType* a  __attribute__((unused)))
{
}
