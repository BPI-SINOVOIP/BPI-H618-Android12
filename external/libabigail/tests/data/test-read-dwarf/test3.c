// Test file for creating multiple alias for a symbol

void __foo(void);
void foo(void) __attribute__((weak, alias("__foo")));
void foo__(void) __attribute__((weak, alias("__foo")));
void __foo__(void) __attribute__((alias("__foo")));

void __foo(void)
{

}
