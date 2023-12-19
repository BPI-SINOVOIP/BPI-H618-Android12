struct S
{
  union
  {
    struct
    {
      struct
      {
	int a;
      };
    };
    int new_union_member;
  };

  int b;
  int c;
  int d;
};

void
foo(struct S* s __attribute__((unused)))
{
}
