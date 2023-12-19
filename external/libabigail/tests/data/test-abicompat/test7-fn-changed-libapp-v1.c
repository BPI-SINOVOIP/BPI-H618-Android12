// Compile this with:
// gcc -g -Wall -fPIC -shared -o libtest7-fn-changed-libapp-v1.so test7-fn-changed-libapp-v1.c
#include <stdio.h>
#include "test7-fn-changed-libapp-v1.h"

int add (float a, float b)
{
   return  a+b;
}

void print (const struct Student s)
{
  printf ("Usn = %d \t Name = %s\t Percentage = %f\n", s.usn, s.name, s.percentage);
}
