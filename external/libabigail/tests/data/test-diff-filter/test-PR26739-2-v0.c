const int global[5]; //#0

typedef const int array_type1[5]; // #1
typedef volatile array_type1 array_type2; // #2

struct A
{
  // The volatile qualifier here applies to the elements of array in
  // declaration #1.  So libabigail's IR should edit the IR for this
  // this qualified type to make the qualifier apply to the element of
  // #1.  All types used for array_type1 should be cloned so that they
  // are not shared with array_type2.
  volatile array_type1 m;
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
f2(array_type2 i)
{
  (void) i;
}

