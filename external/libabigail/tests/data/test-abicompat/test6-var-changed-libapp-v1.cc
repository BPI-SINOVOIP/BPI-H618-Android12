// To compile this type:
//   g++ -g -Wall -shared -o libtest6-var-changed-libapp-v1.so test6-var-changed-libapp-v1.cc
#include "test6-var-changed-libapp-v1.h"

S0::S0()
  : m0(), m1()
{}

S1::S1()
  :m0()
{}

S0* bar = 0;

S1* foo = 0;
