const int global[5]; //#0

typedef const int array_type1[5]; // #1
typedef volatile array_type1 array_type2; // #2

struct A
{
  // Compared to the previous version of this file, the qualifier has
  // been removed below.  So abidiff should report a change only for
  // the type 'struct A'.  Not for array_typ2 declared in #2.  If
  // abidiff reports a change for array_type2, it means it is making a
  // mistake by editing types shared between array_type1 and array_type2.
  array_type1 m;
};

void
f0 (struct A *a)
{
  (void)a;
}

void
f1(array_type1 a)
{
  (void) a;
}

void
f2(array_type2 i) // <-- abidiff should *not* report a change on f2's
		  // use of array_type2.
{
  (void) i;
}
