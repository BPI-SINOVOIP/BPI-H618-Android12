struct S
{
  union
  {
    int tag[3];
    struct
    {
      int a;
      int b;
      int c;
    };
  };

  int d;
};

void
foo(struct S* s __attribute__((unused)))
{
}
