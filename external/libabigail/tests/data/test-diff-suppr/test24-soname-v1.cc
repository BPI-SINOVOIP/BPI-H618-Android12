// Compile this file with:
//   g++ -g -Wall -shared -Wl,-soname=libtest24-soname -o libtest24-soname-v1.so test24-soname-v1.cc

struct S
{
  int	m0;
  char	m1;

  int get() const;
  void set(int);
};

int
S::get() const
{return m0;}

void
S::set(int m)
{m0 = m;}

void
foo(S&)
{
}
