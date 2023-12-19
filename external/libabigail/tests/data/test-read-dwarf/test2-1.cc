///
/// build with g++ -fPIC -g -Wall -shared -o test2.so test2-0.cc test2-1.cc
/// and then pass the resulting test2.so to dwz by doing:
/// dwz test2.so
///
#include "test2.h"

second_type::second_type()
  : member0(0),
    member1(0)
{}

namespace a
{
second_type*
build_second_type()
{return new second_type;}

}
