struct S
{
  unsigned m;

  S();

  int
  mem_fn0();

  int
  mem_fn1();

  int
  mem_fn2();
};

int
S::mem_fn2()
{return 0;}

S::S()
  : m(0)
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
