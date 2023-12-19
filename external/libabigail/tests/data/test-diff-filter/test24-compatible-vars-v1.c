/* Compile with:
     gcc -Wall -g -fPIC -shared -o libtest24-compatible-vars-v1.so test24-compatible-vars-v1.c
 */

struct S
{
  int m0;
};

typedef struct S typedef_of_S;

typedef_of_S a[2];
