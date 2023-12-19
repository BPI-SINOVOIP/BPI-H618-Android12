typedef void * A[7];
typedef A B;
typedef const B C;
typedef C D;
typedef volatile D E;
typedef E F;
typedef restrict F G;
typedef G H;

struct S {
  const A c_a;     // +const
  volatile A v_a;  // +volatile
  restrict A r_a;  // +restrict
  const B c_b;     // +const
  volatile B v_b;  // +volatile
  restrict B r_b;  // +restrict
  const C c_c;     // same
  volatile C v_c;  // +volatile
  restrict C r_c;  // +restrict
  const D c_d;     // same
  volatile D v_d;  // +volatile
  restrict D r_d;  // +restrict
  const E c_e;     // same
  volatile E v_e;  // same
  restrict E r_e;  // +restrict
  const F c_f;     // same
  volatile F v_f;  // same
  restrict F r_f;  // +restrict
  const G c_g;     // same
  volatile G v_g;  // same
  restrict G r_g;  // same
  const H c_h;     // same
  volatile H v_h;  // same
  restrict H r_h;  // same
};

void fun(struct S * s) { (void) s; }
