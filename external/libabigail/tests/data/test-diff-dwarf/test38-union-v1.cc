// Compile this with:
//  g++ -g -Wall -shared -o libtest38-union-v1.so test38-union-v1.cc

struct S
{
  int S_member0;
  int S_member1;
};

union some_union_type
{
  char m1;
  S m2;
};

void
foo(some_union_type*)
{
}
