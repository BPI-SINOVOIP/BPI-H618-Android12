// To compile this, do:
//   g++ -g -shared test27-redundant-and-filtered-children-nodes-v0.cc -o libtest27-redundant-and-filtered-children-nodes-v0.so

struct S0
{
  int m0;
};

struct S1
{
};

struct S2
{
};

struct S
{
  S0* m0;
  S1* m1;
  S2* m2;
};

void
foo(S&)
{
}

S0*
bar(S1*, S2*)
{return 0;}
