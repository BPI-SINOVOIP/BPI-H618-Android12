typedef void (*fn_ptr_type_a_t)(int, int);

typedef void (*fn_ptr_type_a2_t)(int, int);

struct SA
{
  int m1;
  fn_ptr_type_a_t m2;
  fn_ptr_type_a2_t m3;
};

void fun_obja(struct SA*);
