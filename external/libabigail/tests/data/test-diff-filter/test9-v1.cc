struct S
{
  int m0;

  S();

  int
  mem_fn0();

  int
  mem_fn1();
};

S::S()
  : m0(0)
{}

int
S::mem_fn0()
{return 0;}

int
S::mem_fn1()
{return 0;}

void
foo(S&)
{}
