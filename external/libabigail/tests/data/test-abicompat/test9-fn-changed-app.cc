// Compile this with:
//
// gcc -g -Wall -L. -ltest9-fn-changed-v0 -o libtest9-fn-changed-app test9-fn-changed-app.cc
//

#include "test9-fn-changed-v0.h"

int
main()
{
  S s;
  return foo(s);
}
