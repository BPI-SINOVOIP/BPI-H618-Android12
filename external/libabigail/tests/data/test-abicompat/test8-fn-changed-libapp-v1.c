/* Compile with:

   gcc -g -Wall -shared \
   -o libtest8-fn-changed-libapp-v1.so \
   test8-fn-changed-libapp-v1.so
 */

#include "test8-fn-changed-libapp-v1.h"

int
foo(struct S* s)
{return s->m0 + s->m1;}
