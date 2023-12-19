// Compile with:
// g++ -g -Wall -c -fdebug-types-section test23-first-tu.cc
// g++ -g -Wall -c -fdebug-types-section test23-second-tu.cc
// g++ -g -shared -fuse-ld=gold -o libtest23.so test23-first-tu.o test23-second-tu.o
// The secon 
#include <string>
#include <iostream>

using std::cout;
using std::string;

void
emit(string& prefix, string& s)
{
  cout << prefix << ":" << s << "\n";
}
