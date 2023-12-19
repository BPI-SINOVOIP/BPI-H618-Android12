/**
 *
 * Compile with:
 *   gcc -shared -g -Wall -o libtestpr27267-v0.so v0.c
 *
 */

#include "include-dir-v0/include.h"

enum opaque_enum
{
  e0,
  e1
};

struct opaque_struct
{
  opaque_enum m0;
};

void
fn(opaque_struct * s, opaque_enum *e)
{
  s->m0 = *e;
}
