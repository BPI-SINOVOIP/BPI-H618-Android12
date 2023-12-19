// See comments in -v1.c file.
struct sto1 {
  int x;
};

struct sto2 {
  long x;
};

void fn1(struct sto1 s) {
}

void fn2(struct sto2 s) {
}

void fn3(struct sto1* s) {
}

void fn4(struct sto2* s) {
}
