//
// To compile this file, type:
//
// g++ -c -g test44-anon-struct-union-v1.cc
//
//
// This test modifies the type of the S0::m0 data member from the
// anonymous union to a native type. The resulting change report must
// refer to the anonymous union by using its flat representation,
// rather than just "__anonymous_union__".

struct S0
{
  int m0;
  int m1;
};

void
foo(S0*)
{}
