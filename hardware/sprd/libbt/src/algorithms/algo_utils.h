#ifndef __BT_LOG_H__
#define  __BT_LOG_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void big2nd(unsigned char *src,  size_t size);
void dump_hex(const char *tag, unsigned char *dst,  size_t size);
void char2bytes(char* src, unsigned char *dst,  size_t size);
void char2bytes_bg(char* src, unsigned char *dst,  size_t size);
void dump_hex_bg(const char *tag, unsigned char *dst,  size_t size);

#endif
