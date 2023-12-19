#include <linux/module.h>

void exported_function_gpl(void) {}
EXPORT_SYMBOL_GPL(exported_function_gpl);

void local_function(void) {}
int local_variable = 2;
