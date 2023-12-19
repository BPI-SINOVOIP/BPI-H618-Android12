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

struct B : public base
{
  mode		_M_type;
  state		_M_state;
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

struct D5v1 : virtual public C3
{
  int_type	_M_index;
};

struct D5v2 : virtual public C3
{
  int_type	_M_index;
};

// test
struct Dd1 : public A, public B
{ };

struct Dd2 : public A, public D4v
{ };

struct Dd3 : public D2, public D3
{ };

struct Dd4 : public D2v, public D3v
{ };

struct Dd5 : public D5v1, public D5v2
{ };


int main()
{
  Dd1 obj1;
  Dd2 obj2;
  Dd3 obj3;
  Dd4 obj4;
  Dd5 obj5;
  return 0;
}
