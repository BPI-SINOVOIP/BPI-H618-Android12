#include <linux/module.h>

void exported_function(void) {}
EXPORT_SYMBOL(exported_function);

void exported_function_gpl(void) {}
EXPORT_SYMBOL_GPL(exported_function_gpl);

int exported_variable = 1;
EXPORT_SYMBOL(exported_variable);

int exported_variable_gpl = 1;
EXPORT_SYMBOL_GPL(exported_variable_gpl);

void local_function(void) {}
int local_variable = 2;
