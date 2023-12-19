// To compile this type:
//  g++ -g -Wall -o test6-var-changed-app -L. -ltest6-var-changed-libapp-v0 test6-var-changed-app.cc

#include "test6-var-changed-libapp-v0.h"

int
main()
{
  S0* s0 = bar;
  S1* s1 = foo;

  if (s0 && s1)
    return 1;
  return 0;
}
