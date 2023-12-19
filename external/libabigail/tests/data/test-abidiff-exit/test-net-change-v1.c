long var_added = 0;
long fun_added() { return 0; }

long var_changed = 0;
long fun_changed() { return 0; }

struct type_added {
  long x;
};
struct type_changed {
  long x;
};

void victim(struct type_changed * dummy) { (void)dummy->x; }
