struct reachable_struct1
{
  int m0;
};

struct unreachable_struct1
{
  int m1;
};

struct unreachable_struct2
{
  int m1;
};

void foo(struct reachable_struct1* __attribute__((unused)) s)
{
}

static void private_func()
{
  struct unreachable_struct1 s1;
  s1.m1 = 0;

  struct unreachable_struct2 s0;
  s0.m1 = 0;

}
