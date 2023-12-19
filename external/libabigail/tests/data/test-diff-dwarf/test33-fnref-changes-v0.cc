class S
{
	int (&fnref0)(void);
	int (&fnref1)(void);
	int (&fnref2)(void);
};

void foo(S& s)
{
}
typedef int(&callback)(int);

callback fn0(void)
{
}

double(&fn1(void))(int)
{
}

void fn2(callback c)
{
}
