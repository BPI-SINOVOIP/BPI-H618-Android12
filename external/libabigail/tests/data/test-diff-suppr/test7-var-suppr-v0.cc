// Compile this with:
// g++ -Wall -g -shared -Wl,--version-script=test7-var-suppr-version-script -o libtest7-var-suppr-v0.so test7-var-suppr-v0.cc
struct S0
{
  int m0;
};

struct S1
{
  int m0;
};

S0* var0;

asm(".symver var0,var0@VERSION_1.0");

S1* var1;

asm(".symver var1,var1@VERSION_1.0");
