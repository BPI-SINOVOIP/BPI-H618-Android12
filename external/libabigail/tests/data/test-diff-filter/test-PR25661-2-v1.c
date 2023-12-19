struct S
{
  int a;
  struct
  {
    int b;
    struct
    {
      union
      {
	char added_char;
	int c;
      };
    };
  };
  int d;
};

void
foo(struct S* s __attribute__((unused)))
{
}
