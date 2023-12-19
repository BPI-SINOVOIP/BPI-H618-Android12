/* To compile this, type:
   gcc -shared -Wl,--version-script=test12-version-script -o libtest12-v0.so test12-v0.c
 */
int
_foo1(int v)
{return v + 1;}
asm(".symver _foo1,foo@VERSION_1.0");

int
_foo2(int v)
{return v + 2;}
asm(".symver _foo2,foo@@VERSION_2.0");
