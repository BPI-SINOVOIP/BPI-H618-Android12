// Compile with:
//   g++ -g -shared -o libtest25-cyclic-type-v0.so test25-cyclic-type-v0.cc

struct S
{
  int m0;
  S* m2;
};

void
foo(S&)
{
}
