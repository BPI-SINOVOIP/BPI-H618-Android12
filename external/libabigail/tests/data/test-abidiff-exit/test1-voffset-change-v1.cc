// Compile with:
//
//  g++ -g -Wall -c test1-voffset-change-v1.cc
//
class C
{
public:
  virtual char virtual_func1();
  virtual int virtual_func0();
};

int
C::virtual_func0()
{return 0;}

char
C::virtual_func1()
{return 0;}
