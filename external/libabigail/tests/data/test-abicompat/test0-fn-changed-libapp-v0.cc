// Compile with:
//   g++ -fPIC -g -Wall -shared -o libtest0-fn-changed-libapp-v0.so test0-fn-changed-libapp-v0.cc 
//

#include "test0-fn-changed-libapp.h"

namespace libapp
{

struct S0
{
  int m0;

  S0()
    : m0()
  {}
};

struct S1
{
  int m0;

  S1()
    : m0()
  {}
};

struct S2
{
  int m0;

  S2()
    : m0()
  {}
};

struct S3
{
  int m0;

  S3()
    : m0()
  {}
};

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

int
fun0(S0&)
{return 0;}

void
fun1(S1*)
{
}

void
fun2(S2*)
{
}

void
fun3(S3*)
{
}

} // end namespace libapp
