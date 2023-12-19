struct S
{
  int
  foo();
};

int
S::foo()
{
  int i = 0;
  return i;
}

int
bar()
{
  S s;
  return s.foo();
}
