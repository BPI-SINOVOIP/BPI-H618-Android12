// Compile with:
// g++ -g -fPIC -shared -o libtest32-v0.so test32-v0.c

namespace priv
{
  void foo() {}
}

namespace pub
{
  void bar() {}
}
