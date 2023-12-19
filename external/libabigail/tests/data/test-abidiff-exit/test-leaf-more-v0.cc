// See comments in -v1.c file.
struct changed {
  int foo = 0;
};

int deleted_var = 0;
int directly_changed_var = 0;
changed * indirectly_changed_var;

int deleted_fun() {
  return 0;
}
int directly_changed_fun() {
  return 0;
}
void indirectly_changed_fun(changed * x) {
}
