// Compile this *with* optimization so that
// foo is inlined:
//     g++ -g -O2 -c test14-inline-v0.cc

int
foo()
{
  int i = 0;
  ++i;
  return i;
}

int
bar()
{return foo();}
