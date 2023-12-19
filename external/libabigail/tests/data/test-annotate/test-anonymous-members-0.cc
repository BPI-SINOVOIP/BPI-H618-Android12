struct S
{
  struct
  {
    int m;
  };

  struct
  {
    char bar;
  };

  struct
  {
    char baz;
  };

  enum {one, two} e1;

  enum {three, four} e2;

  enum {five, six} e3;

  union {int a; char b;};

  union {unsigned c; double d;};
};

void
foo(S&)
{
}
