typedef const int immutable[7];
typedef immutable monster;

struct A {
  monster q;
};

void fun(struct A * a) { (void) a; }
