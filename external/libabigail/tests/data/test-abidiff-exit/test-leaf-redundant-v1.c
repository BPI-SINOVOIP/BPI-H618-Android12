// Ensure that --redundant is indeed implied by --leaf-changes-only mode and
// that all the changed functions get complete reports.
struct stn1 {
  int x;
};

struct stn2 {
  long x;
  double y;
};

void fn1(struct stn1 s) {  // was sto1
}

void fn2(struct stn2 s) {  // was sto2
}

void fn3(struct stn1* s) {  // was sto1
}

void fn4(struct stn2* s) {  // was sto2
}
