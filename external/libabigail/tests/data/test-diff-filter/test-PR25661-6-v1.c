struct S
{
  union
  {
    int  a;
    char added;
  };
};

void
foo(struct S *s)
{
  s->a = 1;
}
