// Compile this with:
//   g++ -g -Wall -shared -o libtest26-loc-suppr-v1.so test26-loc-suppr-v1.cc
//

#include "test26-loc-suppr.h"

struct S
{
  int m0;
  char added_member;
};

void
func0(S*)
{
}
