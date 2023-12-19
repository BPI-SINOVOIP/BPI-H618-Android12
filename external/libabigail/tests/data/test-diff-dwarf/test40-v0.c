struct S1
{
  int m0;
  char m1;
};

struct S2
{
  int to_remove;
  int m0;
  char m1;
};

struct S3
{
  int to_change;
};

int
foo(struct S1* s)
{
  return s->m0;
}

int
bar(struct S2* s)
{
  return s->m0;
}

int
baz(struct S3* s)
{
  return s->to_change;
}
