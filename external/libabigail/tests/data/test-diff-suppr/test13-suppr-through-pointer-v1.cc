// Compile this with:
//   g++ -g -Wall -shared -o libtest13-suppr-through-pointer-v1.so test13-suppr-through-pointer-v1.cc
struct S
{
  int m0;
  char m1;
};

void
bar(S*)
{
}

void
foo(S)
{
}

void
fu(S**)
{}
