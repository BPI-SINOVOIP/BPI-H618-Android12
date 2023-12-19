// Compile with:
//   g++ -g -Wall -c test27-local-base-diff-v0.cc
struct base
{
  int m0;
};

struct S : public base
{
  int m0;
};

void
bar(base&)
{}

void
foo(S*)
{
}
