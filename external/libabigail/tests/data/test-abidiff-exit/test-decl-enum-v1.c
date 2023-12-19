enum embodied_enum { Y };
enum disembodied_enum;

void reg1(const enum embodied_enum * foo) { (void)foo; }
void reg2(const enum disembodied_enum * foo) { (void)foo; }
