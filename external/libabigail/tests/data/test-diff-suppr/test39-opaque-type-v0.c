#include "test39-public-headers-dir/test39-header-v0.h"

struct _StructType
{
  int m0;
};

void
foo(StructType* a __attribute__((unused)))
{
}

void bar(StructType* a __attribute__((unused)))
{
}
