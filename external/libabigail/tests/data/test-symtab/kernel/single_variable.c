#include <linux/module.h>

int exported_variable = 1;
EXPORT_SYMBOL(exported_variable);

void local_function(void) {}
int local_variable = 2;
