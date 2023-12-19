
// function aliases

void main_func(void);
void alias_func(void) __attribute__((weak, alias("main_func")));

void main_func(void)
{

}

// variables aliases

int main_var;
extern int alias_var __attribute__((weak, alias("main_var")));

