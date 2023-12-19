// Compile with:
//   g++ -g -Wall -c test28-vtable-changes-v1.cc


struct S
{
  virtual void
  bar();

  virtual void
  baz();

  virtual void
  foo();

  virtual ~S();
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

S::~S()
{}
