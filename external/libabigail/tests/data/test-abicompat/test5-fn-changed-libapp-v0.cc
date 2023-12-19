// To compile this type:
//   g++ -g -Wall -shared -o libtest5-fn-changed-libapp-v0.so test5-fn-changed-libapp-v0.cc
#include "test5-fn-changed-libapp-v0.h"

S0::S0()
  : m0()
{}

S1::S1()
  :m0(), m1()
{}

void
bar(S0*)
{}

int
foo(S1*)
{return 0;}
