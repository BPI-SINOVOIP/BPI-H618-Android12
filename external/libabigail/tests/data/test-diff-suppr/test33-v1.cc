/// to compile this:
/// g++ -g -fPIC -shared -o libtest33-v1.so test33-v1.cc

#include "test33-v1.h"

class to_suppress
{
  int member1;

  void method1();
  char method2();
};

void
to_suppress::method1()
{}

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
