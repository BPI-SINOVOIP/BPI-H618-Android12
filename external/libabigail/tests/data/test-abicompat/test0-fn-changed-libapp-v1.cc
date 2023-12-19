// Compile with:
//    g++ -fPIC -g -Wall -shared -o libtest0-fn-changed-libapp-v1.so test0-fn-changed-libapp-v1.cc 
//

#include "test0-fn-changed-libapp.h"

namespace libapp
{

struct S0
{
  int m0;
  char m1;

  S0()
    : m0(),
      m1()
  {}
};

struct S1
{
  int m0;
  char m1;
  unsigned m2;

  S1()
    : m0(),
      m1(),
      m2()
  {}
};

struct S2
{
  int m0;
  unsigned m1;

  S2()
    : m0(),
      m1()
  {}
};

struct S3
{
  int m0;
  unsigned char m1;

  S3()
    : m0(),
      m1()
  {}
};

int
fun0(S0&)
{return 0;}

S0*
create_s0()
{return new S0;}

void
destroy(S0* s)
{delete s;}

S1*
create_s1()
{return new S1;}

void
destroy(S1* s)
{delete s;}

void
fun1(S1*)
{
}

/*
void
fun2(S2*)
{
}

void
fun3(S3*)
{
}
*/

} // end namespace libapp
