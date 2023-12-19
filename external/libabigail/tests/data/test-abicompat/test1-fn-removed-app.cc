// Compile with:
// g++ -g -Wall -L. -ltest1-fn-removed-v0 -o test1-fn-removed-app test1-fn-removed-app.cc

extern void fun1();
extern void fun2();

int
main()
{
  fun1();
  fun2();
  return 0;
}
