struct S
{
  union
  {
    int tag1[2];
    struct
    {
      int a;
      int b;
    };
  };
  union
  {
    int tag2[2];
    struct
    {
      int c;
      int d;
    };
  };
};

void
foo(struct S* s __attribute__((unused)))
{
}
