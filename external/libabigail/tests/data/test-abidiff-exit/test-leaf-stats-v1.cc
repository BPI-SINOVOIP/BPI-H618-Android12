// Test to ensure changed variables and functions are accounted for correctly in
// --leaf-changes-only mode.
long changed_var = 0;  // was int

long changed_fun() {  // was int
  return 0;
}
