// Compile this with:
//   g++ -g -Wall -c test29-finer-redundancy-marking-v0.cc

struct base
{
  int m0;
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
