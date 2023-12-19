/* This is compiled once into test-PR26684-dwarf4.o and once into
   test-PR26684-dwarf5.o:

   gcc -c -gdwarf-5 -o test-PR26684-dwarf5.o test-PR26684.c
   gcc -c -gdwarf-4 -o test-PR26684-dwarf4.o test-PR26684.c

*/
struct pea
{
  int type;
  long a:1, b:1, c:1;
};

struct pea p;
