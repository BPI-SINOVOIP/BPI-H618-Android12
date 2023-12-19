// Compile with:
// g++ -g -Wall -fPIC -shared -o libtest31-v1.so test31-v1.cc

namespace hidden
{

struct S0
{
  int m0;
  char m1;

  S0()
    : m0(), m1()
  {}

  S0(int v)
    : m0(v),
      m1()
  {}
};

void
foo(S0& s)
{
  s.m0 = 2;
  s.m1 = 1;
}

} // end namespace hidden

namespace visible
{
void
bar(int v)
{
  hidden::S0 s(v);
  hidden::foo(s);
}

} // end namespace visible
