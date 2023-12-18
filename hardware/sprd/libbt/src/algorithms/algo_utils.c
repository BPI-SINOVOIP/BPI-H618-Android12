#include <log/log.h>
#include <string.h>
#include "algo_utils.h"


static char* hex_table = "0123456789abcdef";
static inline void byte2hex(char *dst, unsigned char *src) {
  *dst++ = hex_table[(*src >> 4) & 0xf];
  *dst = hex_table[*src & 0xf];
}

static inline unsigned char char2byte(const char* src) {
	if (*src > '0' - 1 && *src < '9' + 1)
		return *src - '0';
	else if (*src > 'A' - 1 && *src < 'F' + 1)
		return *src - 'A' + 10;
	else if (*src > 'a' - 1 && *src < 'f' + 1)
		return *src - 'a' + 10;
	return 0xFF;
}

void big2nd(unsigned char *src,  size_t size)
{
	unsigned char tmp;
	int i, max = (int)(size/2);

	for (i = 0; i < max; i++) {
		tmp = src[i];
		src[i] = src[size - 1 - i];
		src[size - 1 - i] = tmp;
	}
}

void char2bytes(char* src, unsigned char *dst,  size_t size)
{
	while (size -- && src && dst) {
		*dst++ = char2byte(src) << 4 | char2byte(src + 1);
		src += 2;
	}
}

void char2bytes_bg(char* src, unsigned char *dst,  size_t size)
{
	int count = size;
	unsigned char *p = dst;

	while (size -- && src && dst) {
		*dst++ = char2byte(src) << 4 | char2byte(src + 1);
		src += 2;
	}
	big2nd(p, count);
}


void dump_hex(const char *tag, unsigned char *dst,  size_t size)
{
	char *buf = malloc(size * 2);
	unsigned char *tmp = malloc(size);
	char *p = buf;
	unsigned char *q = tmp;

	memcpy(tmp, dst, size);

	while (size--) {
		byte2hex(p, q++);
		p += 2;
	}
	ALOGD("%s %s", tag, buf);
	free(tmp);
	free(buf);
}

void dump_hex_bg(const char *tag, unsigned char *dst,  size_t size)
{
	char *buf = malloc(size * 2);
	unsigned char *tmp = malloc(size);
	char *p = buf;
	unsigned char *q = tmp;

	memcpy(tmp, dst, size);
	big2nd(tmp, size);
	while (size--) {
		byte2hex(p, q++);
		p += 2;
	}
	ALOGD("%s %s", tag, buf);
	free(tmp);
	free(buf);
}
