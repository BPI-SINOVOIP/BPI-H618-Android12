// To compile this type:
//  g++ -g -Wall -o test5-fn-changed-app -L. -ltest5-fn-changed-libapp-v0 test5-fn-changed-app.cc

#include "test5-fn-changed-libapp-v0.h"

int
main()
{
  S0 s0;
  bar(&s0);

  S1 s1;
  return foo(&s1);
}
