int var_removed = 0;
int fun_removed() { return 0; }

int var_changed = 0;
int fun_changed() { return 0; }

struct type_removed {
  int x;
};
struct type_changed {
  int x;
};

void victim(struct type_changed * dummy) { (void)dummy->x; }
