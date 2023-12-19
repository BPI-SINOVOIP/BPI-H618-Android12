class B0S0
{
  int m0;
  char m1;

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
  int m0;
  void member0() const;
};

void
S0::member0() const
{}

class S1 : public B0S1
{
  int m0;
  void member0()
  {}
};

void
foo(S0&, S1*)
{}
