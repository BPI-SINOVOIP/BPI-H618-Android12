/* Compile with:
    gcc -g -Wall -shared -o libtest22-compatible-fns-v1.so test22-compatible-fns-v1.c */

struct S
{
  int m0;
};

typedef struct S typedef_of_S;

void
func0(volatile typedef_of_S const * const * const * p)
{
}
