/* Compile with:
    gcc -g -Wall -shared -o libtest22-compatible-fns-v0.so test22-compatible-fns-v0.c */

struct S
{
  int m0;
};

void
func0(volatile struct S const * const * const * p)
{
}
