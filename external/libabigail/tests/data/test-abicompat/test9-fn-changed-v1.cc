// Compile this with:
//
// gcc -g -Wall -shared -o libtest9-fn-changed-v1.so test9-fn-changed-v1.cc
//
#include "test9-fn-changed-v1.h"

int
foo(S& s)
{return s.m0 + s.m1;}
