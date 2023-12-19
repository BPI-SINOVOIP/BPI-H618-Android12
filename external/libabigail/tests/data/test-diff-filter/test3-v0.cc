struct S
{
  char m1;

  S()
    : m1(0)
  {}
};

int
foo(int i, char)
{return i;}

void
bar(int, S&)
{
}
