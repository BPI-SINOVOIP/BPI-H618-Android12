// Compile this with:
// g++ -Wall -g -shared -o libtest5-fn-suppr-v1.so test5-fn-suppr-v1.cc

struct S
{
  char m0;
  int m1;

  S()
    : m0(),
      m1()
  {}
};

void
bar(S*)
{
}

void
bar(int, S)
{
}
