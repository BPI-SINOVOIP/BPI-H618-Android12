void exported_function(){}

void exported_function_alias() __attribute__((alias("exported_function")));
void exported_function_another_alias() __attribute__((alias("exported_function")));
void exported_function_weak_alias() __attribute__((weak, alias("exported_function")));
void exported_function_another_weak_alias() __attribute__((weak, alias("exported_function")));

int exported_variable = 42;

extern int exported_variable_alias __attribute__((alias("exported_variable")));
extern int exported_variable_another_alias __attribute__((alias("exported_variable")));
extern int exported_variable_weak_alias __attribute__((weak, alias("exported_variable")));
extern int exported_variable_another_weak_alias __attribute__((weak, alias("exported_variable")));
