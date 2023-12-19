// Compile this with:
//
// gcc -g -Wall -shared -o libtest9-fn-changed-v0.so test9-fn-changed-v0.cc
//

#include "test9-fn-changed-v0.h"

int
foo(S& s)
{return s.m0;}
