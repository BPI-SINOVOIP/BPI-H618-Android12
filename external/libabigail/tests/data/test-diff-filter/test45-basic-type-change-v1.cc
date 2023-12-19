// Compile this with:
// g++ -g -shared -o libtest45-basic-type-change-v1.so test45-basic-type-change-v1.cc
//
// This test is to avoid marking the change to int* to char* in the
// data members of struct S1 and S2 as being redundant.  This is
// because those changes are, in the end, basic type changes.  Only
// user-defined types changes should be marked redundant in that
// scenario.

struct S0
{
  char* m0;
};

struct S1
{
  char* m0;
};

struct S2
{
  char* m0;
};

char&
foo0(S0& s)
{return *s.m0;}

char*
foo1(S1&s)
{return s.m0;}

const char
foo2(S2&)
{return 0;}

char
foo3(S2&)
{return 0;}
