// Exercises the reporting of struct differences in --leaf-changes-only mode.
// The resulting report should have no excess blank lines.
struct ops {
  long changed_var;  // was int
  virtual long changed_fn() { return 0; }  // was int
  // Note that in order for this to be treated as an addition, as opposed to a
  // change, we need to make sure it's at a different offset from anything in
  // the old version of ops; having type, name and size different but the same
  // offset won't work.
  long added_var;
  virtual long added_fn() { return 0; }
};

void reg(ops& x) {
  ops instantiate = x;
  (void) instantiate;
}
