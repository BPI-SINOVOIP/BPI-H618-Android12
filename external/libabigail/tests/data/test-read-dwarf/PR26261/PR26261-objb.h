typedef void (*fn_ptr_type_b_t)(int, int, char);

struct SB
{
  int m1;
  fn_ptr_type_b_t m2;
};

void fun_objb(struct SB*);
