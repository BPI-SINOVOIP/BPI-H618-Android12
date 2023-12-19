struct C0
{
  typedef int Int;
  Int m0;
  char m1;

  C0()
    :m0(0),
     m1(0)
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
