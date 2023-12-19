struct priv
{
  unsigned m0;
  char m1;
};

void
init1 (struct priv* p)
{
  p->m0 = 0;
  p->m1 = 0;
}

struct priv*
init2(struct priv* p)
{
  p->m0 = 0xCAFEBABE;
  p->m1 = 0xCA;
  return p;
}
