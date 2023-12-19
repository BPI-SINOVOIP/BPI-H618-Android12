// Compile with:
// g++ -g -shared -o libobj-v0.so obj-v0.cc

struct S0
{
  int mem0;

  S0()
    : mem0()
  {}
};

struct S1
{
};
void
bar(S0&)
{}

void
foo(S1&)
{}
