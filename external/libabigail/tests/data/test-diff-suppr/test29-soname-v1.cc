// Compile this with:
// g++ -g -Wall -shared -Wl,-soname=libtest29-soname -o libtest29-soname-v1.so test29-soname-v1.cc

struct S
{
  int m0;
  char m1;

  static S* sm0;
};

S* S::sm0 = 0;
