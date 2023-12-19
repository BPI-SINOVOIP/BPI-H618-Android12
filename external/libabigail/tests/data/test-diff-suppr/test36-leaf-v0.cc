// Compile this with:
// g++ -g -shared -o libtest36-leaf-v0.so test36-leaf-v0.cc

struct leaf1
{
  int m0;
}; // end struct leaf1

struct leaf2
{
  leaf1 member0;
  char member1;
}; // end struct leaf2

struct struct_type
{
  leaf2* m0;
};

void
interface1(struct_type*)
{}

void
interface2(struct_type&)
{}

void
interface3(struct_type**)
{}
