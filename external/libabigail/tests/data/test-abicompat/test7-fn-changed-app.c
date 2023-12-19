// Compile this with:
// gcc -g -o test7-fn-changed-app -L. -ltest7-fn-changed-libapp-v0 test7-fn-changed-app.c

#include <stdio.h>
#include "test7-fn-changed-libapp-v0.h"

int main()
{
   struct Student s = {7, "foo"};
   float a = 3.5, b = 2.0;
   printf ("Addition of a and b = %f\n",add(a,b));
   print(s);

   return 0;
}
