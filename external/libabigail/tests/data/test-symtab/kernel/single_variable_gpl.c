#include <linux/module.h>

int exported_variable_gpl = 1;
EXPORT_SYMBOL_GPL(exported_variable_gpl);

void local_function(void) {}
int local_variable = 2;
