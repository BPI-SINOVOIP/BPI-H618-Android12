// Compile with:
//   gcc -g -Wall -shared -o libtest25-typedef-v0.so test25-typedef-v0.c

struct PrivateType0
{
  int m0;
};

struct PrivateType1
{
  int m0;
};

typedef struct PrivateType0 Type;

typedef struct PrivateType1* TypePtr;

void
bar(Type *t)
{
  if (t)
    ;
}

void
baz(TypePtr t)
{
  if (t)
    ;
}
