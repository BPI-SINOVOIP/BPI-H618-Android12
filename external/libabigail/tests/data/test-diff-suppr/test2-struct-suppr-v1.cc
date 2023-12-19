struct S
{
  char inserted_char_member;
  int int_member;
};

class C
{
  char inserted_char_member;
  int int_member;
};

void
foo(S&)
{}

void
bar(C&)
{}
