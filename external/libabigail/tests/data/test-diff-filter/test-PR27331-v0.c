struct S {
  int var : 1;
};
int fun(struct S s) { return s.var; }
