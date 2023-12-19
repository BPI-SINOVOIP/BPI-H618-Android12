struct type_to_keep
{
  int m0;
  char m1;
};

struct type_to_suppress
{
  int m0;
  char m1;
};


void
func0(struct type_to_keep)
{
}

void
func1(struct type_to_suppress)
{
}
