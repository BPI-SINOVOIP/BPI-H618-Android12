// See comments in -v1.c file.
struct ops {
  void (*munge)(int x);
};

void register_ops(const ops&) {
}
