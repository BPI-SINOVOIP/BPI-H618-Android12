struct priv
{
  int m0;
};

void
init1(struct priv* p)
{
  p->m0 = 0;
}

struct priv*
init2(struct priv* p)
{
  p->m0 = 0xCAFEBABE;
  return p;
}
