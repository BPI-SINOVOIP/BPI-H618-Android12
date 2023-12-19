class C0
{
  int m0;

public:
  C0()
    : m0(0)
  {}
};

class C1
{
  char m0;
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
