// Compile this *without* optimization so that
// foo is not inlined:
//     g++ -g -O0 -c test14-inline-v0.cc

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
