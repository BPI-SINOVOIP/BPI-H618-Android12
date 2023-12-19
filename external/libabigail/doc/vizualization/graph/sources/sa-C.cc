struct base
{
  enum mode : short { in, out, top, bottom };
  enum state { pass, fail, unknown };
  typedef long int_type;
};

struct C1 : public base
{
  mode		_M_type;
};

// virtual by derivation
struct C2 : virtual public base
{
  mode		_M_type;
};

// virtual by member function
struct C3 : public base
{
  mode		_M_type;

  virtual ~C3() { };
};

// abstract virtual by member function
struct C4 : public base
{
  mode		_M_type;

  virtual void foo() = 0;
};

int main()
{
  C1 obj1;
  C2 obj2;
  C3 obj3;
  //  C4 obj4; XXX abstract
  return 0;
}
