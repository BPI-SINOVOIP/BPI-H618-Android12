// Compile with:
//   g++ -g -Wall -c test14-suppr-non-redundant-v0.cc

struct S
{
  int m0;
};

void
bar(S&)
{}

void
baz(S*)
{}
