// Ensure that local changes to functions and variables (including their removal
// or addition) are reported in --leaf-changes-only mode.
struct changed {
  long foo = 0;  // was int
};

long directly_changed_var = 0;  // was int
changed * indirectly_changed_var;
long added_var = 0;

long directly_changed_fun() {  // was int
  return 0;
}
void indirectly_changed_fun(changed * x) {
}
long added_fun() {
  return 0;
}
