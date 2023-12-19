/// To compile this type:
///   g++ -g -c test45-anon-dm-change-v1.cc
///
/// This test is to detect a change where a non-anonymous data member
/// becomes anonymous and vice-versa.

struct S0
{
  union {int m0; char m01;};
  char m1;
};

struct S1
{
  int m0;
  char m01;
  char m1;
};

void
foo(S0&)
{}

void
bar(S1&)
{}
