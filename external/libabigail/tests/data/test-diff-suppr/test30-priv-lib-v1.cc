// Compile this with:
// g++ -g -Wall -shared -fPIC -o test30-priv-lib-v1.so test30-priv-lib-v1.cc

#include "test30-priv-lib-v1.h"

class opaque_class
{
public:
  virtual int inserted_member();
  virtual int member0();
};

int
opaque_class::member0()
{return 0;}

int
opaque_class::inserted_member()
{return 0;}

int
private_function(opaque_class *o)
{return o->member0() + o->inserted_member();}
