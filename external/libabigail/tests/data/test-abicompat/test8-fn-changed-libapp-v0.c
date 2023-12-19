/* Compile with:

   gcc -g -Wall -shared \
   -o libtest8-fn-changed-libapp-v0.so \
   test8-fn-changed-libapp-v0.c
 */

#include "test8-fn-changed-libapp-v0.h"

int
foo(struct S* s)
{return s->m0;}
