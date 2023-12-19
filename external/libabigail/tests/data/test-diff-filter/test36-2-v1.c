/* To compile this, please type:
 *
 * gcc -g -Wall -shared -o libtest36-v1.so test36-1-v1.c test36-2-v1.c.
 */

struct S
{
  unsigned int m0;
  unsigned int m1;
};

int
foo_2(struct S* s)
{
  return s->m0;
}
