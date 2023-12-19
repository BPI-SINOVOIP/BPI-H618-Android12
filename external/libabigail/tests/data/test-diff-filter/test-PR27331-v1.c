struct S {
  int : 1, var : 1;
};
int fun(struct S s) { return s.var; }
