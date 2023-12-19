// Test for crating symbols having binding STB_GNU_UNIQUE
// build with g++ -fPIC -g -Wall -shared -o test6.so test6.cc

struct A
{
   int i;
};

struct B
{
   int foo()
     {
        static A a;
        return a.i;
     }
};

int
bar()
{
   B b;
   return b.foo();
}

template<typename T>
struct C
{
   static T bar;
};

template<typename T> T C<T>::bar = T();

int
bleh()
{
   C<int> c;
   return c.bar;
}
