// Compile this with :
//  gcc -g -Wall -c test30-vtable-changes-v1.cc

struct S
{
  virtual ~S();
  virtual void fn0();
  virtual void fvtable_breaker();
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

void
S::fvtable_breaker()
{}
