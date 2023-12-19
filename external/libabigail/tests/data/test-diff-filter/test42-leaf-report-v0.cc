// Compile this with:
// g++ -shared -g -o libtest42-leaf-report-v0.so test42-leaf-report-v0.cc

struct leaf
{
  int m0;
};

struct C
{
  leaf *m0;
};

void
fn(C&)
{}
