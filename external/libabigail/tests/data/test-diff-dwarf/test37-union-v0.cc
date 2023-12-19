// Compile this with:
//  g++ -g -Wall -shared -o libtest37-union-v0.so test37-union-v0.cc

union some_union_type
{
  int m0;
  char m1;
};

void
foo(some_union_type*)
{
}
