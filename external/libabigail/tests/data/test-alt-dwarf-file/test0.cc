// To compile this, type:
// g++ -shared -fPIC -g -Wall -L. -ltest0-common -o libtest0.so test0.cc
//
// Once the libtest0-common.so as been generated as well (from the
// test0-common.cc file), please geneate the common debug info file
// using the 'dwz' tool by doing:
//
// dwz -m test0-common-dwz.debug -r libtest0-common.so libtest0.so.
//
// Then, create a build-id subdirectory my doing:
//
// mkdir -p test0-debug-dir/.build-id/<first-two-chars-of-ascii-repr-of-build-id-of-test0-common-dwz.debug>
// Then ln -s `pwd`/test0-common-dwz.debug `pwd`/test0-debug-dir/.build-id/<build-id-of-test0-common-dwz.debug>.debug

#include "test0.h"

S global_s;

unsigned
S::get_m2() const
{return m2;}

void
foo(S& s)
{
  S a = s;
  char c = a.get_m1();
  int i = c + a.get_m0();
  i += (int) a.get_m2();
}

S*
bar()
{
  return &global_s;
}
