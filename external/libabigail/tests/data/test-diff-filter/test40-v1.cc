// compile with:
// g++ -g -shared -fPIC -o libtest40-v1.so test40-v1.cc

struct S
{
  int m;
};

void
foo(const S, int c, bool b)
{
  int d = c + b;
}

void
bar()
{
  S s;
  foo(s, 0, false);
}
