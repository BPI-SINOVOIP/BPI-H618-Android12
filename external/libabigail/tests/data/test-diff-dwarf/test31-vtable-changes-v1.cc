// Compile this with :
//  gcc -g -Wall -c test31-vtable-changes-v1.cc

struct S
{
  virtual ~S();
  virtual void fn1();
};

S::~S()
{}

void
S::fn1()
{}
