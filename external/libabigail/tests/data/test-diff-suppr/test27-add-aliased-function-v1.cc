/*
   Compile this with:

   g++ -g -c -Wall test27-add-aliased-function-v1.cc
 */

void
foo()
{
}

void
bar()
{
}

void
baz()  __attribute__ ((alias("_Z3barv")));
