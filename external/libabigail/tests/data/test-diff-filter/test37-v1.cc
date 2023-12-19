// g++ -g -Wall -shared -o libtest37-v1.so test37-v1.cc

struct A
{
  char m0;
};

struct B
{
  char m0;
};

struct C
{
  char m0;
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
