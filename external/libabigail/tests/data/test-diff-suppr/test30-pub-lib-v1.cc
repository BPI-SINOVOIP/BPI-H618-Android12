// Compile this with:
// g++ -g -Wall -shared -fPIC -o test30-pub-lib-v1.so test30-pub-lib-v1.cc

#include "test30-include-dir-v1/test30-pub-lib-v1.h"
#include "test30-priv-lib-v1.h"

int
private_function(non_defined_class *)
{
  non_defined_class o;
  return o.virtual_func0() + o.virtual_func_inserted();
}

class S::priv_type
{
public:
  non_defined_class* member0;
  int member1;
};

int
public_function(S* s)
{return private_function(s->priv->member0) + s->priv->member1;}
