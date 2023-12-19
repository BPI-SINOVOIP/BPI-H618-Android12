// Compile this with:
// g++ -g -Wall -shared -Wl,-soname=libtest29-soname -o libtest29-soname-v0.so test29-soname-v0.cc

struct S
{
  int m0;

  static S* sm0;
};

S* S::sm0 = 0;
