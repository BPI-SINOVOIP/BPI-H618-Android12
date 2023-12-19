// The change to ops::munge is local to ops and should result in ops being
// reported as a changed type in --leaf-changes-only mode.
struct ops {
  char (*munge)(long x, bool gunk);  // everything but the name changed
};

void register_ops(const ops&) {
}
