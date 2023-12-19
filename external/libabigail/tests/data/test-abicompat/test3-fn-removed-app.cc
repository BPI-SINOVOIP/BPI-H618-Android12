// Compile this with:
//  g++ -Wall -g -L. -ltest3-fn-removed-v0 -o test3-fn-removed-app test3-fn-removed-app.cc 

extern int fun0();
extern void fun1();

int
main()
{
  fun0();
  fun1();
}
