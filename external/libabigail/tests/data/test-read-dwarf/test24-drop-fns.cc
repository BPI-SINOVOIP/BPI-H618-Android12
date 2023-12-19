// Compile this with:
// gcc -g -Wall -fPIC -shared -o libtest24-drop-fns.so test24-drop-fns.cc

#include <iostream>
#include <sstream>
#include <string>

using std::string;
using std::cout;
using std::ostringstream;
using std::ostream;

string
foo(const string& s)
{
  string str = "The input string was: '" + s;
  return str;
}

ostream*
bar(const string& str)
{
  ostringstream *o = new ostringstream;
  *o << str << foo(str);
  return o;
}
