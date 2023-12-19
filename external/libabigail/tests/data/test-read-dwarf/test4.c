// Test file for restrict (needs to be compiled by a GCC that actually emits
// DW_TAG_restrict_type. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59051
//
// gcc -gdwarf-3 -std=c99 -shared -nostartfiles -o test4.so test4.c

char *
cpy (char * restrict s1, const char * restrict s2, unsigned int n)
{
  char *t1 = s1;
  const char *t2 = s2;
  while(n-- > 0)
    *t1++ = *t2++;
  return s1;
}
