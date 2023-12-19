// Compile this with:
//   g++ -g -shared -Wl,--version-script=test18-alias-sym-version-script -o libtest18-alias-sym-v1.so test18-alias-sym-v1.cc

void
foo()
{
}

asm(".symver _Z3foov,foo_alias@VERSION_1.0");

void
foo_another_alias() __attribute__((alias("_Z3foov")));

void
bar()
{
}
