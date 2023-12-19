// Compile this with:
// g++ -g -Wall -c test17-non-refed-syms-v1.cc

void
foo()
{
}

// And below is the definition of a function 'bar', in assembler.
// There won't be any debug info referencing the 'bar' symbol.
asm(".global bar");
asm(".type bar,function");
asm(".text");
asm("bar:");
asm("ret");
