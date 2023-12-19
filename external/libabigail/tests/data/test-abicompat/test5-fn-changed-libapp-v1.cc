// To compile this type:
//   g++ -g -Wall -shared -o libtest5-fn-changed-libapp-v1.so test5-fn-changed-libapp-v1.cc
#include "test5-fn-changed-libapp-v1.h"

S0::S0()
  : m0(), m1()
{}

S1::S1()
  :m0()
{}

void
bar(S0*)
{}

int
foo(S1*)
{return 0;}
