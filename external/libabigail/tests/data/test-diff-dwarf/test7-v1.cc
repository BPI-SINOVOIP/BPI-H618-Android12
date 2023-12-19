struct S
{
  static char m0;
  static int m1;
};

char S::m0;
int S::m1;

void
foo(S&)
{
  S::m0 = 1;
  S::m1 = 2;
}
