/*
 * Compile with:
 *   gcc -shared -g -Wall -o libtest33-v1.so test33-v1.c
 */

#include "test33-pub-include-dir-v1/test33-pub-include-v1.h"
#include "test33-priv-include-dir-v1/test33-priv-include-v1.h"

void
foo(s_ptr ptr, int a)
{
  ptr->p = NULL;
  a++;
}

s_ptr
bar(s_ptr ptr, char a)
{
  ptr->p = NULL;
  a++;
  return ptr;
}

void
baz(s_ptr ptr)
{
  ptr->p = NULL;
}
