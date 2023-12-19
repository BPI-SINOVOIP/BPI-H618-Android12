struct b0
{
  long long m0;
  char m1;
};

struct b1
{
  double m0;
  char m1;
};

struct s0 : virtual b0, virtual b1
{
  typedef int integer;
  typedef unsigned char byte;

  integer m0;
  byte m1;

  s0()
    : m0(0),
      m1(0)
  {
  }

  ~s0()
  {
  }

  integer
  mem_fun();
};

s0::integer
s0::mem_fun()
{
  return m0 + m1;
}

void
foo(s0& s)
{
  s.mem_fun();
}

int
main()
{
  s0 s;
  foo(s);
}
