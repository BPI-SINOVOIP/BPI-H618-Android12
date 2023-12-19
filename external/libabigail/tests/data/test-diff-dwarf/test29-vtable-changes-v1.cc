// Compile this with :
//  gcc -g -Wall -c test29-vtable-changes-v1.cc

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
