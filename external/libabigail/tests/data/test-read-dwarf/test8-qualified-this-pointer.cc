struct S
{
  int i;

  S()
    : i(0)
  {}

  int
  foo() const;
};

int
S::foo() const
{return i;}
