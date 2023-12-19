#include "test1-typedef-suppr.h"

struct a_type
{
  char m_char;
  int m_int;
};

void
foo(a_type* a)
{
  a->m_char = 0;
  a->m_int = 0;
}


struct b_type
{
  char m_char;
  int m_int;
};

void
bar(struct b_type* b)
{
  b->m_char = 0;
  b->m_int = 0;
}
