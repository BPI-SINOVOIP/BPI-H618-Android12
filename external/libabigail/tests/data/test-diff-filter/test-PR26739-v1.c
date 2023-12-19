typedef int plain[7];
typedef const plain monster;

struct A {
  monster q;
};

void fun(struct A * a) {
  (void) a;
  // assignment to read-only location
  // a->q[0] = 0;
}
