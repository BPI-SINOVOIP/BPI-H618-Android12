// Compile with:
// g++ -g -shared -o libobj-v0.so obj-v0.cc

struct S
{
  int mem0;

  S()
    : mem0()
  {}
};

void
bar(S&)
{}
