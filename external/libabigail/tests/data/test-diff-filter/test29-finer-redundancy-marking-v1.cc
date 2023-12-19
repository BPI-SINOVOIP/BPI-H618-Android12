// Compile this with:
//   g++ -g -Wall -c test29-finer-redundancy-marking-v1.cc

struct base
{
  int m0;
  char m1;
};

struct inherited : public base
{
  int m0;
};

void
bar(base&)
{}

void
baz(base)
{}

void
foo(inherited*)
{}
