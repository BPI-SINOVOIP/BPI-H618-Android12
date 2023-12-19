// This file is the source code for 4 binaries.
// It comes from one of the comments of bug libabigail/19173.

// To compile the first two binaries, please do:
//  gcc test35-pr19173-libfoo-long.c -shared -fpic -o test35-pr19173-libfoo-long-gcc.so -g
//  gcc test35-pr19173-libfoo-long.c -shared -fpic -o test35-pr19173-libfoo-long-gcc2.so -g -DLONG
//
// To compile the next two binaries, please do:
// clang test35-pr19173-libfoo-long.c -shared -fpic -o test35-pr19173-libfoo-long-clang.so -g
//  clang test35-pr19173-libfoo-long.c -shared -fpic -o test35-pr19173-libfoo-long-clang2.so -g -DLONG
#ifdef LONG
char buggy_symbol[10];
#else
char buggy_symbol[5];
#endif
