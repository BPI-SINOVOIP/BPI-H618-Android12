struct base
{
  enum mode : short { in, out, top, bottom };
  enum state { pass, fail, unknown };
  typedef long int_type;
};

int main()
{
  base obj;
  return 0;
}
