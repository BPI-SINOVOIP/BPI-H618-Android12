// See comments in -v1.c file.
struct foo {
  int z;
};

struct ops1 {
  int * x;
};

struct ops2 {
  foo y[10];
};

struct ops3 {
  void (*spong)(int & wibble);
};

struct ops4 {
  int * x;
};

struct ops5 {
  int * x;
};

int var6[2][5];

void register_ops1(ops1*) { }
void register_ops2(ops2*) { }
void register_ops3(ops3*) { }
void register_ops4(ops4*) { }
void register_ops5(ops5*) { }
