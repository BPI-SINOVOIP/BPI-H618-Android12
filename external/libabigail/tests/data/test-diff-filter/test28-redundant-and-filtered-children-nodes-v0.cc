// To compile this, type:
//  g++ -Wall -g -shared -o libtest28-redundant-and-filtered-children-nodes-v0.so test28-redundant-and-filtered-children-nodes-v0.cc
class s
{
  int m0;
public:

  s();

  s(const s&);
};

s::s()
  : m0()
{}

s::s(const s& s)
{
  m0 = s.m0;
}

void
foo(s&)
{}

void
bar(s*)
{}
