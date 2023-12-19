// Compile this with:
//  g++ -fPIC -g -Wall -L. -ltest2-var-removed-v0 -o test2-var-removed-app
//  test2-var-removed-app.cc 

extern int global_var1;

int
main()
{
  return global_var1;
}
