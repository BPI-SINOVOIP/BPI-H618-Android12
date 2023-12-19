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

int main()
{
  A obj;
  return 0;
}
