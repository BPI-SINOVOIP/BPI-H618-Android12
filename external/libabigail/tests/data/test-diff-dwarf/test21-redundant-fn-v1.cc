// gcc -g -Wall -shared -o libtest21-redundant-fn-v1.so test21-redundant-fn-v1.cc 

struct S
{
  int m0;
  char m1;
};

void
foo(S*)
{
}

void
bar(S&)
{
}
