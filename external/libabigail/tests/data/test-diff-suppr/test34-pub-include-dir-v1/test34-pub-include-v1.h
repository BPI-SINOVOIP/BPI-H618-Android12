struct priv;
typedef struct priv* priv_ptr;

#define NULL (void*)0

struct S
{
  priv_ptr p;
};

typedef struct S* s_ptr;

void
foo(s_ptr, int a);

s_ptr
bar(s_ptr, char a);

void
baz(s_ptr);
