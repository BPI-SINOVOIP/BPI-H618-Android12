// gcc -g -Wall -shared -o libtest21-redundant-fn-v0.so test21-redundant-fn-v0.cc 

struct S
{
  int m0;
};

void
foo(S*)
{
}

void
bar(S&)
{
}
