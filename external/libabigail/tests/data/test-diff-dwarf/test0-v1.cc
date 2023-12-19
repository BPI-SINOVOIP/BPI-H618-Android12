class B0S0
{
  int m0;
  char m1;
  unsigned m2;

public:
  void
  member0() const;

  static void
  static_member0();
};

void
B0S0::member0() const
{}

void
B0S0::static_member0()
{
}

class B0S1
{
  int m0;

public:
  B0S1()
  {}
};

class S0 : public B0S0
{
protected:
  int m0;

public:
  void
  member0() const;

  int
  get_member0() const;
};

void
S0::member0() const
{}

int
S0::get_member0() const
{return m0;}

class S1 : public B0S1
{
  int m0;
  void member0()
  {}
};

void
foo(S0&, S1*)
{}
