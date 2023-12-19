// Compile with:
//   g++ -g -Wall -c test28-vtable-changes-v0.cc

struct S
{
  void
  bar();

  virtual void
  baz();

  virtual void
  foo();
};

void
S::bar()
{}

void
baz()
{}

void
foo()
{}
