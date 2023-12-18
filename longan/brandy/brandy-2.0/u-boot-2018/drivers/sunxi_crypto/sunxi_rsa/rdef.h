/*
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Based on bitrev from the Linux kernel, by Akinobu Mita
 */

#ifndef _RSA_BASE_H_
#define _RSA_BASE_H_

typedef void *POINTER;
typedef unsigned int UINT4;
typedef unsigned short UINT2;
typedef unsigned char BYTE;

#define MAX_RSA_MODULUS_BITS	2048
#define MAX_RSA_MODULUS_LEN		((MAX_RSA_MODULUS_BITS + 7) / 8)
#define MAX_RSA_PRIME_BITS		((MAX_RSA_MODULUS_BITS + 1) / 2)
#define MAX_RSA_PRIME_LEN		((MAX_RSA_PRIME_BITS + 7) / 8)

#define RSA_ID_OK    0
#define RSA_ID_ERROR 1
#define RSA_RE_LEN  0x0406
#define RSA_RE_DATA 0x0401

#endif
