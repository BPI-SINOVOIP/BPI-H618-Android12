// Compile with:
// g++ -g -shared -o libobj-v0.so obj-v0.cc

struct S0
{
  int  mem0;
  char mem1;

  S0()
    : mem0(),
      mem1()
  {}
};

struct S1
{
  int mem2;
};
void
bar(S0&)
{}

void
foo(S1&)
{}
