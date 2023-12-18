/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include "rdef.h"
#include "rsa.h"
#include "nn.h"

//#define	SUNXI_RSA_DEBUG

/* RSA decryption, according to RSADSI's PKCS #1. */
s32 RSAPublicDecrypt(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		     R_RSA_PUBLIC_KEY *publicKey)
{
	s32 status;
	u8 pkcsBlock[MAX_RSA_MODULUS_LEN];
	u32 i, modulusLen, pkcsBlockLen;

	modulusLen = (publicKey->bits + 7) / 8;
	if (inputLen > modulusLen)
		return (RSA_RE_LEN);

	status = rsapublicfunc(pkcsBlock, &pkcsBlockLen, input, inputLen,
			       publicKey);
	if (status)
		return (status);

	if (pkcsBlockLen != modulusLen)
		return (RSA_RE_LEN);

	/* Require block type 1. */
	if ((pkcsBlock[0] != 0) || (pkcsBlock[1] != 1))
		return (RSA_RE_DATA);

	for (i = 2; i < modulusLen - 1; i++) {
		if (*(pkcsBlock + i) != 0xff)
			break;
	}

	/* separator check */
	if (pkcsBlock[i++] != 0)
		return (RSA_RE_DATA);

	*outputLen = modulusLen - i;
	if (*outputLen + 11 > modulusLen)
		return (RSA_RE_DATA);

	memcpy((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);

	/* Clear sensitive information. */
	memset((POINTER)pkcsBlock, 0, sizeof(pkcsBlock));

	return RSA_ID_OK;
}

/* RSA encryption, according to RSADSI's PKCS #1. */
s32 RSAPrivateEncrypt(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		      R_RSA_PRIVATE_KEY *privateKey)
{
	s32 status;
	u8 pkcsBlock[MAX_RSA_MODULUS_LEN];
	u32 i, modulusLen;

	modulusLen = (privateKey->bits + 7) / 8;
	if (inputLen + 11 > modulusLen)
		return (RSA_RE_LEN);

	*pkcsBlock = 0;
	/* block type 1 */
	*(pkcsBlock + 1) = 1;
	for (i			 = 2; i < modulusLen - inputLen - 1; i++)
		*(pkcsBlock + i) = 0xff;

	/* separator */
	pkcsBlock[i++] = 0;
	memcpy((POINTER)&pkcsBlock[i], (POINTER)input, inputLen);
	status = rsaprivatefunc(output, outputLen, pkcsBlock, modulusLen,
				privateKey);
	/* Clear sensitive information. */
	memset((POINTER)pkcsBlock, 0, sizeof(pkcsBlock));

	return status;
}

/* RSA decryption, according to RSADSI's PKCS #1. */
s32 RSAPrivateDecrypt(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		      R_RSA_PRIVATE_KEY *privateKey)
{
	s32 status;
	u8 pkcsBlock[MAX_RSA_MODULUS_LEN];
	u32 i, modulusLen, pkcsBlockLen;

	modulusLen = (privateKey->bits + 7) / 8;
	if (inputLen > modulusLen)
		return (RSA_RE_LEN);

	status = rsaprivatefunc(pkcsBlock, &pkcsBlockLen, input, inputLen,
				privateKey);
	if (status)
		return (status);

	if (pkcsBlockLen != modulusLen)
		return (RSA_RE_LEN);

	/* We require block type 2. */
	if ((*pkcsBlock != 0) || (*(pkcsBlock + 1) != 2))
		return (RSA_RE_DATA);

	for (i = 2; i < modulusLen - 1; i++) {
		/* separator */
		if (*(pkcsBlock + i) == 0)
			break;
	}

	i++;
	if (i >= modulusLen)
		return (RSA_RE_DATA);

	*outputLen = modulusLen - i;
	if (*outputLen + 11 > modulusLen)
		return (RSA_RE_DATA);

	memcpy((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);
	/* Clear sensitive information. */
	memset((POINTER)pkcsBlock, 0, sizeof(pkcsBlock));

	return RSA_ID_OK;
}

/* Raw RSA public-key operation. Output has same length as modulus.
   Requires input < modulus.
*/
s32 rsapublicfunc(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		  R_RSA_PUBLIC_KEY *publicKey)
{
	NN_DIGIT c[MAX_NN_DIGITS], e[MAX_NN_DIGITS], m[MAX_NN_DIGITS],
		n[MAX_NN_DIGITS];
	u32 eDigits, nDigits;

	/* decode the required RSA function input data */
	NN_Decode(m, MAX_NN_DIGITS, input, inputLen);
	NN_Decode(n, MAX_NN_DIGITS, publicKey->modulus, MAX_RSA_MODULUS_LEN);
	NN_Decode(e, MAX_NN_DIGITS, publicKey->exponent, MAX_RSA_MODULUS_LEN);

	nDigits = NN_Digits(n, MAX_NN_DIGITS);
	eDigits = NN_Digits(e, MAX_NN_DIGITS);
	pr_debug("nDigits = %d, eDigits = %d\n", nDigits, eDigits);

	if (NN_Cmp(m, n, nDigits) >= 0)
		return (RSA_RE_DATA);

	*outputLen = (publicKey->bits + 7) / 8;
	/* Compute c = m^e mod n.  To perform actual RSA calc.*/
	NN_ModExp(c, m, e, eDigits, n, nDigits);
	/* encode output to standard form */
	NN_Encode(output, *outputLen, c, nDigits);

	/* Clear sensitive information. */
	memset((POINTER)c, 0, sizeof(c));
	memset((POINTER)m, 0, sizeof(m));

	return RSA_ID_OK;
}

/* Raw RSA private-key operation. Output has same length as modulus.
   Requires input < modulus.
*/

s32 rsaprivatefunc(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		   R_RSA_PRIVATE_KEY *privateKey)
{
	NN_DIGIT c[MAX_NN_DIGITS], cP[MAX_NN_DIGITS], cQ[MAX_NN_DIGITS],
		dP[MAX_NN_DIGITS], dQ[MAX_NN_DIGITS], mP[MAX_NN_DIGITS],
		mQ[MAX_NN_DIGITS], n[MAX_NN_DIGITS], p[MAX_NN_DIGITS],
		q[MAX_NN_DIGITS], qInv[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
	u32 cDigits, nDigits, pDigits;

	/* decode required input data from standard form */
	NN_Decode(c, MAX_NN_DIGITS, input, inputLen); /* input */

	/* private key data */
	NN_Decode(p, MAX_NN_DIGITS, privateKey->prime[0], MAX_RSA_PRIME_LEN);
	NN_Decode(q, MAX_NN_DIGITS, privateKey->prime[1], MAX_RSA_PRIME_LEN);
	NN_Decode(dP, MAX_NN_DIGITS, privateKey->primeExponent[0],
		  MAX_RSA_PRIME_LEN);
	NN_Decode(dQ, MAX_NN_DIGITS, privateKey->primeExponent[1],
		  MAX_RSA_PRIME_LEN);
	NN_Decode(n, MAX_NN_DIGITS, privateKey->modulus, MAX_RSA_MODULUS_LEN);
	NN_Decode(qInv, MAX_NN_DIGITS, privateKey->coefficient,
		  MAX_RSA_PRIME_LEN);
	/* work out lengths of input components */
	cDigits = NN_Digits(c, MAX_NN_DIGITS);
	pDigits = NN_Digits(p, MAX_NN_DIGITS);
	nDigits = NN_Digits(n, MAX_NN_DIGITS);

	if (NN_Cmp(c, n, nDigits) >= 0)
		return (RSA_RE_DATA);

	*outputLen = (privateKey->bits + 7) / 8;
	/* Compute mP = cP^dP mod p  and  mQ = cQ^dQ mod q. (Assumes q has
	   length at most pDigits, i.e., p > q.) */

	NN_Mod(cP, c, cDigits, p, pDigits);
	NN_Mod(cQ, c, cDigits, q, pDigits);

	NN_AssignZero(mP, nDigits);
	NN_ModExp(mP, cP, dP, pDigits, p, pDigits);

	NN_AssignZero(mQ, nDigits);
	NN_ModExp(mQ, cQ, dQ, pDigits, q, pDigits);

	/* Chinese Remainder Theorem:
	   m = ((((mP - mQ) mod p) * qInv) mod p) * q + mQ. */

	if (NN_Cmp(mP, mQ, pDigits) >= 0) {
		NN_Sub(t, mP, mQ, pDigits);
	} else {
		NN_Sub(t, mQ, mP, pDigits);
		NN_Sub(t, p, t, pDigits);
	}

	NN_ModMult(t, t, qInv, p, pDigits);
	NN_Mult(t, t, q, pDigits);
	NN_Add(t, t, mQ, nDigits);

	/* encode output to standard form */
	NN_Encode(output, *outputLen, t, nDigits);

	/* Clear sensitive information. */
	memset((POINTER)c, 0, sizeof(c));
	memset((POINTER)cP, 0, sizeof(cP));
	memset((POINTER)cQ, 0, sizeof(cQ));
	memset((POINTER)dP, 0, sizeof(dP));
	memset((POINTER)dQ, 0, sizeof(dQ));
	memset((POINTER)mP, 0, sizeof(mP));
	memset((POINTER)mQ, 0, sizeof(mQ));
	memset((POINTER)p, 0, sizeof(p));
	memset((POINTER)q, 0, sizeof(q));
	memset((POINTER)qInv, 0, sizeof(qInv));
	memset((POINTER)t, 0, sizeof(t));
	return RSA_ID_OK;
}

#define MAXLEN 130
#define HEX 16
#define DEC 10
typedef struct bignum {
	unsigned int m_length;
	unsigned int m_value[MAXLEN];
} bignum;

void bignum_init(bignum *bignumber)
{
	int i;
	bignumber->m_length = 1;
	for (i			      = 0; i < MAXLEN; i++)
		bignumber->m_value[i] = 0;
}

/****************************************************************************************
大数比较
调用方式：Cmp(N,A)
返回值：若N<A返回-1；若N=A返回0；若N>A返回1
****************************v************************************************************/
int Cmp(bignum *N, bignum *A)
{
	int i;
	if (N->m_length > A->m_length)
		return 1;
	if (N->m_length < A->m_length)
		return -1;
	for (i = N->m_length - 1; i >= 0; i--) {
		if (N->m_value[i] > A->m_value[i])
			return 1;
		if (N->m_value[i] < A->m_value[i])
			return -1;
	}
	return 0;
}

/****************************************************************************************
大数赋值
调用方式：Mov(N,A)
返回值：无，N被赋值为A
****************************************************************************************/
void Mov(bignum *to, bignum *from)
{
	int i;
	to->m_length = from->m_length;
	for (i		       = 0; i < MAXLEN; i++)
		to->m_value[i] = from->m_value[i];
}
void Mov_l(bignum *to, u64 A)
{
	int i;
	if (A > 0xffffffff) {
		to->m_length   = 2;
		to->m_value[1] = (unsigned long)(A >> 32);
		to->m_value[0] = (unsigned long)A;
	} else {
		to->m_length   = 1;
		to->m_value[0] = (unsigned long)A;
	}
	for (i		       = to->m_length; i < MAXLEN; i++)
		to->m_value[i] = 0;
}

/****************************************************************************************
大数相加
调用形式：Add(N,A,X)
返回值：X=N+A
****************************************************************************************/

void Add(bignum *N, bignum *A, bignum *X)
{
	int carry	      = 0, i;
	unsigned long long sum = 0;
	Mov(X, N);
	if (X->m_length < A->m_length)
		X->m_length = A->m_length;
	for (i = 0; i < X->m_length; i++) {
		sum	   = A->m_value[i];
		sum	   = sum + X->m_value[i] + carry;
		X->m_value[i] = (unsigned long)sum;
		carry	 = (unsigned)(sum >> 32);
	}
	X->m_value[X->m_length] = carry;
	X->m_length += carry;
}

void Add_l(bignum *N, unsigned long A, bignum *X)
{
	unsigned long long sum;
	Mov(X, N);
	sum = X->m_value[0];
	sum += A;
	X->m_value[0] = (unsigned long)sum;
	if (sum > 0xffffffff) {
		unsigned i = 1;
		while (X->m_value[i] == 0xffffffff) {
			X->m_value[i] = 0;
			i++;
		}
		X->m_value[i]++;
		if (X->m_length == i)
			X->m_length++;
	}
}

/****************************************************************************************
大数相减
调用形式：Sub(N,A,X)
返回值：X=N-A
****************************************************************************************/
void Sub(bignum *N, bignum *A, bignum *X)
{
	unsigned carry = 0;
	unsigned long long num;
	int i;
	Mov(X, N);
	if (Cmp(X, A) <= 0)
		Mov_l(X, 0);
	else {
		for (i = 0; i < N->m_length; i++) {
			if ((N->m_value[i] > A->m_value[i]) ||
			    ((N->m_value[i] == A->m_value[i]) &&
			     (carry == 0))) {
				X->m_value[i] =
					N->m_value[i] - carry - A->m_value[i];
				carry = 0;
			} else {
				num	   = 0x100000000 + N->m_value[i];
				X->m_value[i] = (unsigned long)(num - carry -
								A->m_value[i]);
				carry = 1;
			}
		}
		while (X->m_value[X->m_length - 1] == 0)
			X->m_length--;
	}
}

void Sub_l(bignum *N, unsigned long A, bignum *X)
{
	unsigned long long num = 0x100000000 + X->m_value[0];
	int i		       = 1;
	Mov(X, N);
	if (X->m_value[0] >= A) {
		X->m_value[0] -= A;
	} else if (X->m_length == 1) {
		Mov_l(X, 0);
	} else {
		X->m_value[0] = (unsigned long)(num - A);
		while (X->m_value[i] == 0) {
			X->m_value[i] = 0xffffffff;
			i++;
		}
		X->m_value[i]--;
		if (X->m_value[i] == 0)
			X->m_length--;
	}
}

/****************************************************************************************
大数相乘
调用形式：Mul(N,A,X)
返回值：X=N*A
****************************************************************************************/
void Mul(bignum *N, bignum *A, bignum *X)
{
	unsigned long long sum, mul = 0, carry = 0;
	int i, j;

	if (A->m_length == 1) {
		unsigned long long mul;
		unsigned long carry = 0;
		Mov(X, N);
		if (A->m_value[0] == 0)
			Mov_l(X, 0);
		else {
			for (i = 0; i < N->m_length; i++) {
				mul	   = N->m_value[i];
				mul	   = mul * A->m_value[0] + carry;
				X->m_value[i] = (unsigned long)mul;
				carry	 = (unsigned long)(mul >> 32);
			}
			if (carry) {
				X->m_length++;
				X->m_value[X->m_length - 1] = carry;
			}
		}
	} else {
		X->m_length = N->m_length + A->m_length - 1;
		for (i = 0; i < X->m_length; i++) {
			sum   = carry;
			carry = 0;
			for (j = 0; j < A->m_length; j++) {
				if (((i - j) >= 0) && ((i - j) < N->m_length)) {
					mul   = N->m_value[i - j];
					mul = mul * A->m_value[j];
					carry += mul >> 32;
					mul = mul & 0xffffffff;
					sum += mul;
				}
			}
			carry += sum >> 32;
			X->m_value[i] = (unsigned long)sum;
		}
		if (carry) {
			X->m_length++;
			X->m_value[X->m_length - 1] = (unsigned long)carry;
		}
	}
}
void Mul_l(bignum *N, unsigned long A, bignum *X)
{
	unsigned long long mul;
    unsigned long carry = 0;
    unsigned i = 0;
	if (A == 0)
		Mov_l(X, 0);
	else {
		Mov(X, N);
		for (i = 0; i < N->m_length; i++) {
			mul	   = N->m_value[i];
			mul	   = mul * A + carry;
			X->m_value[i] = (unsigned long)mul;
			carry	 = (unsigned long)(mul >> 32);
		}
		if (carry) {
			X->m_length++;
			X->m_value[X->m_length - 1] = carry;
		}
	}
}
/****************************************************************************************
大数求模
调用形式：
返回值：X=N%A
****************************************************************************************/
void Mod(bignum *N, bignum *A, bignum *X)
{
	unsigned long long div, num;
	int i, len;
	bignum Y, middle;
	bignum_init(&Y);
	bignum_init(&middle);
	Mov(X, N);
	while (Cmp(X, A) >= 0) {
		div = X->m_value[X->m_length - 1];
		num = A->m_value[A->m_length - 1];
		len = X->m_length - A->m_length;
		if ((div == num) && (len == 0)) {
			Sub(X, A, &middle);
			Mov(X, &middle);
			break;
		}
		if ((div <= num) && len) {
			len--;
			div = (div << 32) + X->m_value[X->m_length - 2];
		}
		div = div / (num + 1);
		Mov_l(&Y, div);
		Mul(A, &Y, &middle);
		Mov(&Y, &middle);
		if (len) {
			(&Y)->m_length += len;
			for (i = (&Y)->m_length - 1; i >= len; i--) {
				(&Y)->m_value[i] = (&Y)->m_value[i - len];
			}
			for (i			 = 0; i < len; i++)
				(&Y)->m_value[i] = 0;
		}

		Sub(X, &Y, &middle);
		Mov(X, &middle);
	}
}

void Mod_l(bignum *N, unsigned long A, bignum *X)
{
	unsigned long long div;
	unsigned long carry = 0;
	int i;

	if (N->m_length == 1)
		X->m_value[0] = N->m_value[0] % A;
	else {
		for (i = N->m_length - 1; i >= 0; i--) {
			div = N->m_value[i];
			div += carry * 0x100000000;
			carry = (unsigned long)(div % A);
		}
		X->m_value[0] = carry;
	}
}
/****************************************************************************************
从字符串按10进制或16进制格式输入到大数
调用格式：Get(str,sys,N)
返回值：N被赋值为相应大数
sys暂时只能为10或16
****************************************************************************************/
void Get(char c[513], unsigned int system, bignum *to)
{
	int k;
	int i, j;
	bignum X;
	bignum_init(&X);
	Mov_l(to, 0);
	for (j = 0; c[j] != '\0'; j++)
		;
	for (i = 0; i < j; i++) {
		Mul_l(to, system, &X);
		Mov(to, &X);
		if ((c[i] >= '0') && (c[i] <= '9'))
			k = c[i] - 48;
		else if ((c[i] >= 'A') && (c[i] <= 'F'))
			k = c[i] - 55;
		else if ((c[i] >= 'a') && (c[i] <= 'f'))
			k = c[i] - 87;
		else
			k = 0;
		Add_l(to, k, &X);
		Mov(to, &X);
	}
}

/****************************************************************************************
将大数按16进制格式输出为字符串
调用格式：Put(N)
返回值：无，参数str被赋值为N的16进制字符串
sys暂时只能为10或16
****************************************************************************************/

void Put(bignum *N)
{
#ifdef SUNXI_RSA_DEBUG
	unsigned int i;
	for (i = 0; i < N->m_length; i++) {
		printf("%08x ", N->m_value[N->m_length - i - 1]);
	}
	printf("\n");
#endif
}

void dump_hex(u8 *buf, u32 len)
{
#ifdef SUNXI_RSA_DEBUG
	int i = 0;
	for (i = 0; i < len; i++) {
		printf("%02x ", buf[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
#endif
}

s32 sunxi_rsa_calc_with_software(u8 *n_addr, u32 n_len, u8 *e_addr, u32 e_len,
				 u8 *dst_addr, u32 dst_len, u8 *src_addr,
				 u32 src_len)
{
	bignum E, N, S, D;
	u32 eDigits, nDigits;
	int i;

	bignum_init(&E);
	bignum_init(&N);
	bignum_init(&S);
	bignum_init(&D);

	pr_debug("%s start\n", __func__);

	for (i = 0; i < src_len; i++) {
		memcpy((void *)((char *)S.m_value + src_len - i - 1),
		       src_addr + i, 1);
	}
	S.m_length = src_len / 4;

	for (i = 0; i < n_len; i++) {
		memcpy((void *)((char *)N.m_value + n_len - i - 1), n_addr + i,
		       1);
	}
	N.m_length = n_len / 4;

	memcpy((void *)E.m_value, e_addr, e_len);
	E.m_length = 1;

	pr_debug("nlen=%d, elen = %d\n", N.m_length, E.m_length);
	nDigits = NN_Digits(N.m_value, MAX_NN_DIGITS);
	eDigits = NN_Digits(E.m_value, MAX_NN_DIGITS);
	pr_debug("nDigits = %d, eDigits = %d\n", nDigits, eDigits);

	NN_ModExp(D.m_value, S.m_value, E.m_value, eDigits, N.m_value, nDigits);
	pr_debug("decryption data\n");
	D.m_length = 8;
	Put(&D);

	for (i = 0; i < (D.m_length * 4); i++) {
		memcpy(dst_addr + i,
		       (char *)D.m_value + (D.m_length * 4) - i - 1, 1);
	}

	pr_debug("...........D...........::%d\n", dst_len);
	pr_debug("%s end\n", __func__);
	return 0;
}
