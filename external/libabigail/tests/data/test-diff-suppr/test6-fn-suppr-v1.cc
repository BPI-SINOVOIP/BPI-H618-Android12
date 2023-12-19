// Compile this with:
// g++ -Wall -g -shared -Wl,--version-script=test6-fn-suppr-version-script -o libtest6-fn-suppr-v1.so test6-fn-suppr-v1.cc

struct base
{
  char m0;
  char m1;

  base()
    : m0(), m1()
  {}
};

struct S : public base
{
  int m0;

  S()
    : m0()
  {}
};

struct S1 : private base
{
  int m0;

  S1()
    : m0()
  {}
};

struct S2 : private base
{
  int m0;

  S2()
    : m0()
  {}
};

int
bar(S&)
{return 0;}

asm(".symver _Z3barR1S,_Z3barR1S@VERSION_1.0");

void
bar(S1*)
{}

char
bar(int, S2**)
{return 0;}
