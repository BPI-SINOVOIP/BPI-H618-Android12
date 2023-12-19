// Compile this with:
// gcc -g -Wall -fPIC -shared -o libtest7-fn-changed-libapp-v0.so test7-fn-changed-libapp-v0.c

#include <stdio.h>
#include "test7-fn-changed-libapp-v0.h"

float add (float a, float b)
{
   return  a+b;
}

void print (const struct Student s)
{
  printf ("Usn = %d \t Name = %s\n", s.usn, s.name);
}
