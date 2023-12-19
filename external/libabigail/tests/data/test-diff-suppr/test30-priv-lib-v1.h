// This is a class that, when #included in a .cc file will generate a
// dwarf representation that is declaration-only class because the all
// of the methods of the class are inline virtual methods but one.
// And the one virtual method that is not inline is not defined.  So
// the .cc file that is going to define that method is going to see
// the class as being defined, and that file will also have the
// definition of the vtable.
class non_defined_class
{
 public:
  non_defined_class(){};
  virtual ~non_defined_class(){}
  virtual int virtual_func_inserted(){return 0;};
  virtual int virtual_func1();
  virtual int virtual_func0(){return 0;}
};

int private_function(non_defined_class *);
