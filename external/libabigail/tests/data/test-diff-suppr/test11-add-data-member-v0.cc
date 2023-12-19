// To compile this:
//  g++ -g -Wall -shared -o libtest11-add-data-member-v0.so test11-add-data-member-v0.cc 
struct S
{
  char m0;
};

int
foo(S* s)
{return !!s;}
