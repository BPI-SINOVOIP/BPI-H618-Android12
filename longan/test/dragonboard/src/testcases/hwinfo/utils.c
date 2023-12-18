#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include "minIni/dev/minIni.h"
#include "utils.h"

static struct tm *getlocaltime(void)
{
	time_t t;
	time(&t);
	return localtime(&t);
}

int log_print(int level, const char *tag, const char *fmt, ...)
{
	struct tm *lt;
	int color;
	char banner;
	va_list arg;

	if (level < LEVEL_DEF)
		return 0;

	lt = getlocaltime();
	va_start(arg, fmt);

	switch (level) {
		case LEVEL_E:
			color  = 31;
			banner = 'E';
			break;
		case LEVEL_W:
			color  = 33;
			banner = 'W';
			break;
		case LEVEL_I:
			color  = 32;
			banner = 'I';
			break;
		case LEVEL_D:
			color  = 34;
			banner = 'D';
			break;
		case LEVEL_V:
			color  = 30;
			banner = 'V';
			break;
		default:
			color  = 30;
			banner = 'U';
			break;
	}

	fprintf(stdout, "%02d-%02d %02d:%02d:%02d %5d %c %-10s: \033[48;%dm",
			lt->tm_mon,
			lt->tm_mday,
			lt->tm_hour,
			lt->tm_min,
			lt->tm_sec,
			getpid(),
			banner,
			tag, color);

	vfprintf(stdout, fmt, arg);

	fprintf(stdout, "\033[0m\n");

	va_end(arg);

	return 0;
}

int property_get(const char *name, char *value, const char *defaults)
{
	return ini_gets(PROPERTY_SECTION, name, defaults, value, PROPERTY_VALUE_MAX, PROPERTY_FILE);
}

int property_set(const char *name, char *value)
{
	return ini_puts(PROPERTY_SECTION, name, value, PROPERTY_FILE);
}
