// Compile this with:
// g++ -shared -g -o libtest42-leaf-report-v1.so test42-leaf-report-v1.cc

struct leaf
{
  int m0;
  char m1;
};

struct C
{
  leaf *m0;
};

void
fn(C&)
{}
