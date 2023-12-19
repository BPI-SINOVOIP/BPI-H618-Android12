// Compile this with:
// g++ -Wall -g -shared -o libtest5-fn-suppr-v0.so test5-fn-suppr-v0.cc

struct S
{
  int m1;

    S()
    : m1()
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
