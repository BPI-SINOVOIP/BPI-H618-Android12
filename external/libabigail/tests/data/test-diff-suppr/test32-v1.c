// Compile with:
// g++ -g -fPIC -shared -o libtest32-v1.so test32-v1.c

namespace priv
{
  int foo() {return 0;}
}

namespace pub
{
  char bar() {return 0;}
}
