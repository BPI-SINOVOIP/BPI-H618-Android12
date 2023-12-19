struct S0
{
  int* m0;
};

typedef const char* POINTER;

struct S1
{
  POINTER m0;
};

void foo(struct S0* a __attribute__((unused)))
{
}

void bar(struct S1* a __attribute__((unused)))
{
}
