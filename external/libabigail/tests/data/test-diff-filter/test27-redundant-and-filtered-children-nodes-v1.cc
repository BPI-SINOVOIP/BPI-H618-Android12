// To compile this, do:
//  g++ -g -shared test27-redundant-and-filtered-children-nodes-v1.cc -o libtest27-redundant-and-filtered-children-nodes-v1.so
struct S0
{
  int m0;
  char m1;
};

typedef S0 S0Renamed;

struct S1
{
};

typedef S1 S1Renamed;

struct S2
{
};

typedef S2 S2Renamed;

struct S
{
  S0* m0;
  S1Renamed* m1;
  S2Renamed* m2;
};

void
foo(S&)
{
}

S0*
bar(S1Renamed*, S2Renamed*)
{return 0;}
