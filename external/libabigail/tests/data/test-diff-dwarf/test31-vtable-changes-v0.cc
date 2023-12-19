// Compile this with :
//  gcc -g -Wall -c test31-vtable-changes-v0.cc

struct S
{
  virtual ~S();
  virtual void fn0();
  virtual void fn1();
};

S::~S()
{}

void
S::fn0()
{}

void
S::fn1()
{}
