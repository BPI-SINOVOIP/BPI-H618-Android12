struct S
{
  typedef char Char;
  int m0;
  Char m1;

  S()
    : m0(0),
      m1(0)
  {}
};

typedef int Int;

Int
foo(Int i, char)
{return i;}

void
bar(int, S&)
{
}
