struct MyType
{
  struct Private;
  Private *priv;
};

struct MyType::Private
{
  int m0;
};


void foo(MyType *s)
{}
