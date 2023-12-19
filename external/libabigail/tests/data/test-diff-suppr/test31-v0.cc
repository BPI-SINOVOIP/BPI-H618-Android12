// Compile with:
// g++ -g -Wall -fPIC -shared -o libtest31-v0.so test31-v0.cc

namespace hidden
{

struct S0
{
  int m0;

  S0() {}

  S0(int v)
    : m0(v)
  {}
};

void
foo(S0& s)
{
  s.m0 = 2;
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
