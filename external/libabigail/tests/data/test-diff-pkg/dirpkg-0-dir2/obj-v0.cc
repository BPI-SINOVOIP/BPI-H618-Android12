// Compile with:
// g++ -g -shared -o libobj-v0.so obj-v0.cc

struct S
{
  int  mem0;
  char mem1;

  S()
    : mem0(),
      mem1()
  {}
};

void
bar(S&)
{}
