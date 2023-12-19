// Compile this with :
//  gcc -g -Wall -c test29-vtable-changes-v0.cc

struct S
{
  virtual ~S();
  virtual void fn0();
};

S::~S()
{}

void
S::fn0()
{}
