// compile with:
//   bg++ -g -Wall -L. -ltest4-soname-changed-v0 -o test4-soname-change-app test4-soname-change-app.cc

extern void func0();

int
main()
{
  func0();
}
