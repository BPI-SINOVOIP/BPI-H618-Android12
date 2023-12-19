struct S0
{
  char m0;
  int m1;

  S0()
    : m0(0),
      m1(0)
  {}
};

struct S1
{
  int
  member_fn0(S0&);
};

int
S1::member_fn0(S0&)
{return 0;}

void
foo(S1&)
{}
