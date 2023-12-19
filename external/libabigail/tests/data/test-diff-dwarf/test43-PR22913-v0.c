typedef struct
{
  char m0;
} * Struct0Ptr;

char
f0(Struct0Ptr s)
{return s->m0;}

typedef struct
{
  char m1;
  char m2;
} * Struct1Ptr;

char
f1(Struct1Ptr s)
{return s->m1;}
