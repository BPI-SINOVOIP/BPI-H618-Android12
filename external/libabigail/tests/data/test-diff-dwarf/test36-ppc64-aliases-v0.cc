// This source code must be compiled on a ppc64 machine!!
// The compilation command must be:
//
// g++ -g -Wall -O2 -shared -o libtest36-ppc64-aliases-v0.so test36-ppc64-aliases-v0.cc

struct S
{
  S();
  S(int);
  ~S();
  int m0;
};

S::S()
  : m0()
{}

S::S(int s)
  : m0(s)
{}

S::~S()
{}

void
foo(S&)
{}
