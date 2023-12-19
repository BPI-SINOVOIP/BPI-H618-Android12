class E0
{
  char m0;
public:
  E0();
};

E0::E0()
  : m0(0)
{}

class E1
{
  char m0;
public:
  E1();
};

E1::E1()
  : m0(0)
{}

class C0
{
  char m2;
  E0 m0;
  E1 m1;

public:
  C0();
};

C0::C0()
  : m2(0)
{}

class C1
{
  char m0;
  int m1;

public:
  C1();
};

C1::C1()
  : m0(0),
    m1(0)
{}

void
foo(C0&)
{
}

void
bar(C1&)
{
}
