class S
{
	int (*fnptr0)(void);
	int (*fnptr1)(void);
	int (*fnptr2)(void);

};

void foo(S s)
{
}

typedef int(*callback)(int);

callback fn0(void)
{
}

double(*fn1(void))(int)
{
}

void fn2(callback c)
{
}

