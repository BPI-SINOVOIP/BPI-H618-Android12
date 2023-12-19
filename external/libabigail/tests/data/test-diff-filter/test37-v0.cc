// g++ -g -Wall -shared -o libtest37-v0.so test37-v0.cc

struct A
{
  int m0;
};

struct B
{
  int m0;
};

struct C
{
  int m0;
};

void
f1(A&)
{}

void
f2(B&)
{}

void
f3(C&)
{}
