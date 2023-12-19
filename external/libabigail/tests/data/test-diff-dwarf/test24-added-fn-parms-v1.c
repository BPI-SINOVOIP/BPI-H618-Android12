// To compile this type:
//  gcc -g -Wall -shared -o libtest24-added-fn-parms-v1.so test24-added-fn-parms-v1.c
void
fun()
{
}

void
fun(__attribute__((unused))unsigned arg0,
    __attribute__((unused)) char arg1,
    __attribute__((unused)) int arg2)
{
}
