int
_foo1(int v)
{return v + 1;}
asm(".symver _foo1,foo@VERSION_1.0");

int
_foo2(int v)
{return v + 2;}
asm(".symver _foo2,foo@@VERSION_2.0");
