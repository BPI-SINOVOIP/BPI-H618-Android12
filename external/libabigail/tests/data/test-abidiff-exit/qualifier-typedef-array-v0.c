typedef void * A[7];
typedef A B;
typedef const B C;
typedef C D;
typedef volatile D E;
typedef E F;
typedef restrict F G;
typedef G H;

struct S {
  A c_a;
  A v_a;
  A r_a;
  B c_b;
  B v_b;
  B r_b;
  C c_c;
  C v_c;
  C r_c;
  D c_d;
  D v_d;
  D r_d;
  E c_e;
  E v_e;
  E r_e;
  F c_f;
  F v_f;
  F r_f;
  G c_g;
  G v_g;
  G r_g;
  H c_h;
  H v_h;
  H r_h;
};

void fun(struct S * s) { (void) s; }
