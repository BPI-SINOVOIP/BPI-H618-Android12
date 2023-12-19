struct buzz {
  long a;
};

struct flexible {
  long count;
  struct buzz lightyear[0];
};

struct flexible var;
void fun(struct flexible flex) { (void) flex; }

