/* To compile this, please type:
 *
 * gcc -g -Wall -shared -o libtest36-v0.so test36-1-v0.c test36-2-v0.c.
 */

struct S
{
  int m0;
};

int
foo_1(struct S* s)
{
  return s->m0;
}
