#include <linux/module.h>

void exported_function(void) {}
EXPORT_SYMBOL(exported_function);

void local_function(void) {}
int local_variable = 2;
