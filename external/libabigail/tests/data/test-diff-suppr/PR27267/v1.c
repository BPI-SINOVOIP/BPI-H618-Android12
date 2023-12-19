/**
 *
 * Compile with:
 *   gcc -shared -g -Wall -o libtestpr27267-v1.so v1.c
 *
 */

#include "include-dir-v1/include.h"

enum opaque_enum
{
  e0,
  e1,
  e2
};

struct opaque_struct
{
  opaque_enum m0;
  opaque_enum m1;
};

void
fn(opaque_struct * s, opaque_enum *e)
{
  s->m0 = *e;
  s->m1 = s->m0;
}
