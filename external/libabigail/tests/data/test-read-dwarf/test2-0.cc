///
/// build with g++ -fPIC -g -Wall -shared -o test2.so test2-0.cc test2-1.cc
/// and then pass the resulting test2.so to dwz by doing:
/// dwz test2.so
///
#include "test2.h"

first_type::first_type()
  : member0(0),
    member1(0)
{}
namespace a
{

first_type*
build_first_type()
{return new first_type;}

}
