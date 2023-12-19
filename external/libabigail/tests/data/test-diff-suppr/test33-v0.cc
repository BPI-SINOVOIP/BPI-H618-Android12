/// to compile this:
/// g++ -g -fPIC -shared -o libtest33-v0.so test33-v0.cc

#include "test33-v0.h"

class to_suppress
{
  int method1();
  char method2();
};

int
to_suppress::method1()
{return 0;}

char
to_suppress::method2()
{return 0;}

void
to_keep::method1()
{}

void
to_keep::method2()
{}

void
foo(to_suppress&)
{}

void
bar(to_keep&)
{}
