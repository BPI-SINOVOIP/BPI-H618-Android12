union S
{
  void bar() const
  {
  }

  int needed;
};

void
fun(S s)
{
  s.bar();
}
