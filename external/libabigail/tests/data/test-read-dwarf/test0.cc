// Random comment
//
int global = 1;

namespace ns0
{
long
foo(char* c, const volatile long l);


void
bar(const int c, ...)
{}

void
baz(int& c)
{c = 0;}

enum E {e0, e1};

void
bar2(E& e)
{int c = e; ++c;}

typedef long long long_long;

long_long
baz2(int& c)
{c = 0; return c;}

typedef const volatile unsigned long long useless_long_long;

static useless_long_long
this_should_not_be_seen_by_bidw()
{
  int i = 0;
  bar(0);
  baz2(i);
  return 0;
}

} // end namespace ns0

long
ns0::foo(char* c, const volatile long l)
{return *c + l;}

int
main()
{
  return ns0::this_should_not_be_seen_by_bidw();
}
