// Compile with:
//  g++ -g -Wall -shared -o libtest21-compatible-vars-v1.so test21-compatible-vars-v1.cc

struct S
{
  int m0;
};

typedef S typedef_of_S;

volatile typedef_of_S const * const * const * var0;
