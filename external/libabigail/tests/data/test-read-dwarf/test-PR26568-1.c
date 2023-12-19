struct A {
  union {
    struct {
      int x;
    };
    struct {
      long y;
    };
  };
};

void fun(struct A * a) {
  a->x = 0;
  a->y = 0x0102030405060708ULL;
}
