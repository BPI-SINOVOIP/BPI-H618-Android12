/*
   Compile this with:

   gcc -g -c -Wall test28-add-aliased-function-v1.c
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
baz()  __attribute__ ((alias("bar")));
