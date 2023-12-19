/* This has to be compiled on a ppc64le machine using:
   gcc -g -Wall -shared -o libtest32-struct-change-v0.so test32-ppc64le-struct-change-v0.c
 */

struct S
{
  int m0;
};


int
bar(struct S* s)
{return s->m0;}

int
foo(struct S *s)
{return s->m0;}
