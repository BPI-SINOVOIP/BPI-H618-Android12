struct embodied { int y; };
struct disembodied;

void reg1(const struct embodied * foo) { (void)foo; }
void reg2(const struct disembodied * foo) { (void)foo; }
