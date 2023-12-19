struct S1 {
  int a;
  union {
    int b;
    float c;
  };
  int d;
};

struct S2 {
  int a;
  struct {
    int b;
    float c;
  };
  int d;
};

int
foo(struct S1* p)
{return p->b;}

int
bar(struct S2* p)
{return p->b;}
