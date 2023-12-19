// Compile with:
//  g++ -g -Wall -shared -o libtest21-compatible-vars-v0.so test21-compatible-vars-v0.cc 

struct S
{
  int m0;
};

volatile S const * const * const * var0;
