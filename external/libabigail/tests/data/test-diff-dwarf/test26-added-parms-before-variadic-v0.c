/* gcc -g -Wall -shared -o libtest26-added-parms-before-variadic-v0.so test26-added-parms-before-variadic-v0.c  */
void
fun1(const char* c, ...)
{
  if (c)
    c++;
}

void
fun2(const char* c, ...)
{
  if (c)
    c++;
}

void
fun3(const char* c, const int* d, ...)
{
  if (c)
    c++;

  if (d)
    d++;
}

void
fun4(const char* c, const int* d, ...)
{
  if (c)
    c++;

  if (d)
    d++;
}
