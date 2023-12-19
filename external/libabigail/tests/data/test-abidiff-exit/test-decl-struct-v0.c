struct embodied;
struct disembodied { int x; };

void reg1(const struct embodied * foo) { (void)foo; }
void reg2(const struct disembodied * foo) { (void)foo; }
