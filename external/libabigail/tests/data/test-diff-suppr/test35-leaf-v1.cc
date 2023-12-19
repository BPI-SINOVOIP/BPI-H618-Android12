// Compile this with:
//
// g++ -shared -g -o libtest35-leaf-v1.so test35-leaf-v1.cc

struct leaf
{
  int m0;
  char m1;
};

struct leaf_to_filter
{
  int member0;
  int added;
};

struct C
{
  leaf *m0;
  leaf_to_filter *m1;
};

void
fn(C&)
{}
