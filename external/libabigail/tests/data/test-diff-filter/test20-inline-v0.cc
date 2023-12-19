struct S
{
  int
  foo()
  {
    int i = 0;
    ++i;
    return i;
  }
};

int
bar()
{
  S s;
  return s.foo();
}
