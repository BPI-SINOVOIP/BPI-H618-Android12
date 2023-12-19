// Compile this with GCC's C compiler using:
//    gcc -g -c -Wall -c test1-typedef-suppr-v0.c
//

#include "test1-typedef-suppr.h"

struct a_type
{
  int m_int;
};

void
foo(a_type* a)
{a->m_int = 0;}

struct b_type
{
  int m_int;
};

void
bar(struct b_type* b)
{b->m_int = 0;}
