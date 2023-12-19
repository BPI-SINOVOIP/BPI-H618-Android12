struct C0
{
  typedef int Int;
  Int m0;

  C0()
    :m0(0)
  {}
 };

struct C1
{
  void
  foo(C0&);
};

void
C1::foo(C0&)
{}
