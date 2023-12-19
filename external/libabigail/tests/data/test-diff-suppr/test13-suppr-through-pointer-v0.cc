// Compile this with:
//   g++ -g -Wall -shared -o libtest13-suppr-through-pointer-v0.so test13-suppr-through-pointer-v0.cc

struct S
{
  int m0;
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
