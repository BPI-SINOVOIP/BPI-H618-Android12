// Compile this with:
//  g++ -g -Wall -shared -o libtest39-union-v0.so test39-union-v0.cc

struct S
{
  int S_member0;
  int S_member1;
};

union some_union_type
{
  int m0;
  char m1;
  S m2;
};

void
foo(some_union_type*)
{
}
