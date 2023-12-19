/// To compile this type:
///   g++ -g -c test45-anon-dm-change-v0.cc
///
/// This test is to detect a change where a non-anonymous data member
/// becomes anonymous and vice-versa.

struct S0
{
  int m0;
  char m1;
};

struct S1
{
  struct {int m0; char m01;};
  char m1;
};

void
foo(S0&)
{}

void
bar(S1&)
{}
