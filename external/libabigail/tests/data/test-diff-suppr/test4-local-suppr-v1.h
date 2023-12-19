struct private_opaque_data;
struct public_type
{
  unsigned oops; // <--- we accidentally added a member here.  This
		 // breaks ABI.
  struct private_data* priv_;
};

struct a_not_private_type
{
  int i;
  char j; // <-- This added member should be flagged too.
};

void
foo(struct public_type* p, struct a_not_private_type* t);
