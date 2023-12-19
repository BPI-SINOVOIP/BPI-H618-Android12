struct C0
{
  int m0;

  C0()
    :m0(0)
  {}
};

typedef C0 c0_type;

c0_type
foo()
{return C0();}
