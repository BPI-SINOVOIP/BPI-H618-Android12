// To compile this type:
//
// g++ -shared -g -Wall -o libtest0-common.so test0-common.cc

#include "test0.h"

S::S()
  :m0(0), m1(0), m2(0)
{}

int
S::get_m0() const
{return m0;}

char
S::get_m1() const
{return m1;}
