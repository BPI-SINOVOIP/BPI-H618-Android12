// To compile this:
//  g++ -g -Wall -shared -o libtest12-add-data-member-v1.so test12-add-data-member-v1.cc 
struct S
{
  char m0;
  char m_inserted1;
  int m1;
  char m_inserted2;
};

int
foo(S* s)
{return !!s;}
