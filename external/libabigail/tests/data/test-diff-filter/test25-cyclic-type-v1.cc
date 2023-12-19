// Compile with:
//   g++ -g -shared -o libtest25-cyclic-type-v1.so test25-cyclic-type-v1.cc

struct S
{
  int m0;
  char m1;
  S* m2;
};

void
foo(S&)
{
}
