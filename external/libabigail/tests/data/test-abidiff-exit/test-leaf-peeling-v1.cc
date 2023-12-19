// Ensure that different pointer / reference types are considered genuinely
// different in --leaf-changes-only mode.
struct foo {
  long z;  // was int
};

struct ops1 {
  int ** x;  // was *
};

struct ops2 {
  // A change to foo's size (impacting y's size) is currently considered local
  // here.  Arguably this should be considered non-local as the change to foo is
  // also being reported independently.  If this happens, the test case will
  // need to be updated (to remove the reporting of the ops2 diff).
  foo y[10];
};

struct ops3 {
  void (*spong)(int && wibble);  // was &
};

struct ops4 {
  int & x;  // was *
};

struct ops5 {
  int *** x;  // was *
};

// TODO: This *should* be considered a local change, but currently is not.
int var6[5][2];  // was [2][5]

void register_ops1(ops1*) { }
void register_ops2(ops2*) { }
void register_ops3(ops3*) { }
void register_ops4(ops4*) { }
void register_ops5(ops5*) { }
