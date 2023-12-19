struct private_opaque_data;
struct public_type
{
  struct private_data* priv_;
};

struct a_not_private_type
{
  int i;
};

void
foo(struct public_type* p, struct a_not_private_type* t);
