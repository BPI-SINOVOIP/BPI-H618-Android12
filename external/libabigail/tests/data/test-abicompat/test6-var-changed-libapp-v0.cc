// To compile this type:
//   g++ -g -Wall -shared -o libtest6-var-changed-libapp-v0.so test6-var-changed-libapp-v0.cc
#include "test6-var-changed-libapp-v0.h"

S0::S0()
  : m0()
{}

S1::S1()
  :m0(), m1()
{}

S0* bar = 0;

S1* foo = 0;
