// See comments in -v1.c file.
struct ops {
  int changed_var;
  virtual int changed_fn() { return 0; }
  int deleted_var;
  virtual int deleted_fn() { return 0; }
};

void reg(ops& x) {
  ops instantiate = x;
  (void) instantiate;
}
