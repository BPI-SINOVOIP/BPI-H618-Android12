struct MyType
{
  struct Private;
  Private *priv;
};

struct MyType::Private
{
  int m0;
  char m1;
};


void foo(MyType* s)
{}
