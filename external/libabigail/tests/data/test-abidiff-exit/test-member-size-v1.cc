struct M;

struct S {
  int x;
  M * m;
  int y;
};

struct M {
  int a;
  S * s;
};

struct T {
  S s;
  int a;
};

struct U {
  struct {
    S s;
  };
  int r;
};

void reg1(S*, T*, T*) { }
void reg2(U*) { }
