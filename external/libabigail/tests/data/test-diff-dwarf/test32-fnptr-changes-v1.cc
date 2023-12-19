class S
{
	int (*fnptr0)(double);
	// distinct kinds
	void* fnptr1;
	int fnptr2;

};

void foo(S s)
{
}

typedef void(*callback)(int);

callback fn0(void)
{
}

int(*fn1(void))(int)
{
}

void fn2(callback c)
{
}

