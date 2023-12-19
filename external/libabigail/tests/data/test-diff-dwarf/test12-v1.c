/* To compile this, type:
   gcc -shared -Wl,--version-script=test12-version-script -o libtest12-v1.so test12-v1.c
 */
int
_foo1(int v)
{return v + 1;}
asm(".symver _foo1,foo@VERSION_1.0");

int
_foo2(int v)
{return v + 2;}
asm(".symver _foo2,foo@VERSION_2.0");

int
_foo3(int v, int w)
{return v + w + !2;}
asm(".symver _foo3,foo@@VERSION_3.0");
