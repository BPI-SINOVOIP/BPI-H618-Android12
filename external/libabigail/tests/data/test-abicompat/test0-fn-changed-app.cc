// Compile with:
//    g++  -g -Wall -o test0-fn-changed-app -L. -ltest0-fn-changed-libapp-v0 test0-fn-changed-app.cc
//

#include "test0-fn-changed-libapp.h"

int
main()
{
  libapp::S0* s0 = libapp::create_s0();
  libapp::S1* s1 = libapp::create_s1();

  fun0(*s0);
  fun1(s1);

  libapp::destroy(s0);
  libapp::destroy(s1);
}
