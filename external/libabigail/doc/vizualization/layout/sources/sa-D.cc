struct base
{
  enum mode : short { in, out, top, bottom };
  enum state { pass, fail, unknown };
  typedef long int_type;
};

struct A : public base
{
  int_type	_M_i;
  int_type	_M_n;
  mode		_M_type;
};

struct C1 : public base
{
  mode		_M_type;
};

struct C2 : virtual public base
{
  mode		_M_type;
};

struct C3 : public base
{
  mode		_M_type;

  virtual ~C3() { };
};

struct C4 : public base
{
  mode		_M_type;

  virtual void foo() = 0;
};

struct D1 : public A
{
  int_type	_M_index;
  int_type*	_M_array;
  bool		_M_ok;
};

struct D2 : public C1
{
  state		_M_state;
  int_type	_M_index;
  int_type*	_M_array;
  bool		_M_ok;
};

struct D3 : public C1
{
  int_type	_M_index;
  int_type*	_M_array;
};

struct D2v : virtual public C2
{
  state		_M_state;
  int_type	_M_index;
  int_type*	_M_array;
  bool		_M_ok;
};

struct D3v : virtual public C3
{
  int_type	_M_index;
  int_type*	_M_array;
};

struct D4v : public C4
{
  int_type	_M_index;
  int_type*	_M_array;

  void foo() { }
};

int main()
{
  D1 obj1;
  D2 obj2;
  D3 obj3;
  D2v obj2v;
  D3v obj3v;
  D4v obj4v;
  return 0;
}
