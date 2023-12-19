// To compile this, type:
//  g++ -Wall -g -shared -o libtest28-redundant-and-filtered-children-nodes-v1.so test28-redundant-and-filtered-children-nodes-v1.cc

class s
{
  int m0;
  char m1;

  s(const s&);

public:

  s();

};

s::s()
  : m0()
{}

s::s(const s& s)
{
  m0 = s.m0;
  m1 = s.m1;
}
void
foo(s&)
{}

void
bar(s*)
{}
