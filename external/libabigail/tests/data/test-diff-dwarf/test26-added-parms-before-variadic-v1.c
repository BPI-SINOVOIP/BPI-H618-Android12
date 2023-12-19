/* gcc -g -Wall -shared -o libtest26-added-parms-before-variadic-v1.so test26-added-parms-before-variadic-v1.c  */
void
fun1(const char* c, const char* c1, ...)
{
  if (c)
    c++;

  if (c1)
    c1++;
}

void
fun2(const char* c, const char* c1, ...)
{
  if (c)
    c++;

  if (c1)
    c1++;
}

void
fun3(const char* c, const int* d, const int* d1, ...)
{
  if (c)
    c++;

  if (d)
    d++;

    if (d1)
    d1++;
}

void
fun4(const char* c, const int* d, const int* d1, ...)
{
  if (c)
    c++;

  if (d)
    d++;

    if (d1)
    d1++;
}
