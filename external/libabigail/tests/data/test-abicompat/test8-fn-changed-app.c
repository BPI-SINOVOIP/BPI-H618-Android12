/* Compile with:

   gcc -g -Wall -L. -ltest8-fn-changed-libapp-v0 \
   -o test8-fn-changed-app test8-fn-changed-app.c
 */

#include <string.h>
#include "test8-fn-changed-libapp-v0.h"

int
main()
{
  struct S s;
  memset(&s, 0, sizeof(s));
  int r = foo(&s);
  return r;
}
