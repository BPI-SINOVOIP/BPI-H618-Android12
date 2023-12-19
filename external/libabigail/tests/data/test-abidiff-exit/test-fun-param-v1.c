struct ops {
  void(*foo)(void);
  void(*bind_class)(void *, unsigned int, unsigned long, void *, unsigned long);
  int(*bar)(int);
};

void reg(struct ops* o) { (void) o; }
