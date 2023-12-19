struct E0
{
  char m0;
  E0();
};

E0::E0()
  : m0(0)
{}

struct E1
{
  char m0;
  E1();
};

E1::E1()
  : m0(0)
{}

struct C0
{
  E0 m0;
  E1 m1;
};

class C1
{
  char m0;
public:
  C1();
};

C1::C1()
  : m0(0)
{}

void
foo(C0&)
{
}

void
bar(C1&)
{
}
