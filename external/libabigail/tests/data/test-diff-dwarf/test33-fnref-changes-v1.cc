class S
{
	int (&fnref0)(double);
	// distinct kinds
	int* &fnref1;
	int &fnref2;
};

void foo(S& s)
{
}
typedef void(&callback)(int);

callback fn0(void)
{
}

int(&fn1(void))(int)
{
}

void fn2(callback c)
{
}
