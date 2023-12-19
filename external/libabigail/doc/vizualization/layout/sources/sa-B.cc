struct base
{
  enum mode : short { in, out, top, bottom };
  enum state { pass, fail, unknown };
  typedef long int_type;
};

struct B : public base
{
  mode		_M_type;
  state		_M_state;
};

int main()
{
  B obj;
  return 0;
}
