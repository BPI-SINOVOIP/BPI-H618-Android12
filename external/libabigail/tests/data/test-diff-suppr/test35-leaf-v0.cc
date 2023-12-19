// Compile this with:
//
// g++ -shared -g -o libtest35-leaf-v0.so test35-leaf-v0.cc

struct leaf
{
  int m0;
};

struct leaf_to_filter
{
  int member0;
};

struct C
{
  leaf *m0;
  leaf_to_filter *m1;
};

void
fn(C&)
{}
