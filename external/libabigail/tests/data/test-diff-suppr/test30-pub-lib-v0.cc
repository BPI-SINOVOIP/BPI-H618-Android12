// Compile this with:
// g++ -g -Wall -shared -fPIC -o test30-pub-lib-v0.so test30-pub-lib-v0.cc

#include "test30-include-dir-v0/test30-pub-lib-v0.h"
#include "test30-priv-lib-v0.h"

int
private_function(non_defined_class*)
{
  non_defined_class o;
  return o.virtual_func0() + o.virtual_func_to_be_removed();
}

class S::priv_type
{
public:
  non_defined_class* member0;
};

int
public_function(S* s)
{return private_function(s->priv->member0);}
