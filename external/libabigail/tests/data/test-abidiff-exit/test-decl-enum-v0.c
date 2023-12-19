enum embodied_enum;
enum disembodied_enum { X };

void reg1(const enum embodied_enum * foo) { (void)foo; }
void reg2(const enum disembodied_enum * foo) { (void)foo; }
