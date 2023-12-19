struct B0
{
  char m0;

  B0();
};

B0::B0()
  : m0(0)
{}

struct S : public B0
{
  int m0;

  S();
};

S::S()
  : m0(0)
{}

void
foo(S&)
{}
