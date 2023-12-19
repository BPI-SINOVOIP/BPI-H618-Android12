// To compile this run:
//   gcc -g -Wall -shared -o libtest26-qualified-redundant-node-v1.so test26-qualified-redundant-node-v1.cc
//

struct FooStruct
{
  struct Embedded;
  int m0;
};

struct FooStruct::Embedded
{
  int m0;
  char m1;
  void* m2;
};

struct S
{
  int m0;
  FooStruct::Embedded* m1;
  FooStruct::Embedded** m2;
};

typedef S TypedefOfS;

typedef const TypedefOfS ConstTypedefOfS;

void
foo(ConstTypedefOfS*)
{
}
