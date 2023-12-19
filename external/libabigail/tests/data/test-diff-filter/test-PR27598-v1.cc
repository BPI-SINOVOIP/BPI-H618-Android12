union S
{
  void bar() const
  {
  }
};

void
fun(S s)
{
  s.bar();
}
