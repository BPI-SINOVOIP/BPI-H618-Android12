class C0
{
  typedef int Int;
  Int m0;

  C0();

public:
  void
  foo(const C0&);
};

C0::C0()
  :m0(0)
{}

void
C0::foo(const C0&)
{
}

void
foo(const C0&)
{
}

struct C1
{
  void
  bar(C0&);
};

void
C1::bar(C0&)
{}
