/*
 * Compile this test with:
 *   gcc -c -g test37-opaque-type-v0.c
 *
 *   This test exhibits changes that are redundant in a weird way.
 *   The redundant path through the diff graph involves typedefs,
 *   function types and function parameter diff nodes.
 *
 */
#include "test37-opaque-type-header-dir/test37-opaque-type-header-v0.h"

struct opaque_struct
{
  int m0;
  int m1;
  struct public_struct_type *m2;
};

void
bar0(public_struct_pointer_type *p  __attribute__((unused)))
{
}
