// To compile this:
//  g++ -g -Wall -shared -o libtest12-add-data-member-v0.so test12-add-data-member-v0.cc 
struct S
{
  char m0;
  int m1;
};

int
foo(S* s)
{return !!s;}
