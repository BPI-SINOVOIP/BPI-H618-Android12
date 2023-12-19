// To compile this:
//  g++ -g -Wall -shared -o libtest11-add-data-member-v1.so test11-add-data-member-v1.cc
struct S
{
  char m0;
  char m1;
  int m2;
};

int
foo(S* s)
{return !!s;}
