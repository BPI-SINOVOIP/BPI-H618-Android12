struct C0
{
  int m0;

  C0()
    : m0(0)
  {}
};

class C1
{
  int m1;

public:
  C1()
    : m1(0)
  {}
};

void
foo(C0&)
{}

void
bar(C1*)
{}
