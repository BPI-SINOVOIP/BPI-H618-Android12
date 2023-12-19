/**
 *
 * In this test case, we want the first parameter of the function
 * wrapped_call to be represented in the output abixml by a pointer to
 * the same function type as the one pointed to by the types
 * fn_ptr_type_a_t and fn_ptr_type_b_t.
 *
 * This means that we want the function type pointed by these three
 * function pointer to be the same, as a result of the
 * canonicalization of the DIEs that represent these three function
 * types.
 *
 */
#include "PR26261-obja.h"
#include "PR26261-objb.h"

void
wrapped_call(void (*fun)(int, int),
	     int a,
	     int b)
{
  if (fun)
    fun(a, b);
}

int
main(int argc, char ** argv)
{
  struct SA sa = {0, 0, 0};
  struct SB sb = {0, 0};

  sa.m1 = 0;
  sb.m1 = 0;

  fun_obja(&sa);
  fun_objb(&sb);
  wrapped_call(sa.m2, sa.m1, sb.m1);

  return 0;
}
