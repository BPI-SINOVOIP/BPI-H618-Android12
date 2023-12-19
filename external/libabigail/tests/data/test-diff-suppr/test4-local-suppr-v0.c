// To compile this, type:
// gcc -shared -g -Wall -o libtest4-local-suppr-v0.so test4-local-suppr-v0.c

#include "test4-local-suppr-v0.h"

struct private_data
{
  int private_data0;
};

void
foo(struct public_type* p __attribute__((unused)),
    struct a_not_private_type* t __attribute__((unused)))
{
  /* Do something with p */
}
