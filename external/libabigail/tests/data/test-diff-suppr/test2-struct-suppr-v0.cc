struct S
{
  int int_member;
};

class C
{
  int int_member;
};

void
foo(S&)
{}

void
bar(C&)
{}
