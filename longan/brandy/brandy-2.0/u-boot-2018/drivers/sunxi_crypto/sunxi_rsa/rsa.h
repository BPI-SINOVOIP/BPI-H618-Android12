/*
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Based on bitrev from the Linux kernel, by Akinobu Mita
 */

#ifndef _RSA_C_H_
#define _RSA_C_H_

typedef struct {
	u32 bits; /* length in bits of modulus */
	u8 modulus[MAX_RSA_MODULUS_LEN]; /* modulus */
	u8 exponent[MAX_RSA_MODULUS_LEN]; /* public exponent */
} R_RSA_PUBLIC_KEY;

typedef struct {
	u32 bits; /* length in bits of modulus */
	u8 modulus[MAX_RSA_MODULUS_LEN]; /* modulus */
	u8 publicExponent[MAX_RSA_MODULUS_LEN]; /* public exponent */
	u8 exponent[MAX_RSA_MODULUS_LEN]; /* private exponent */
	u8 prime[2][MAX_RSA_PRIME_LEN]; /* prime factors */
	u8 primeExponent[2][MAX_RSA_PRIME_LEN]; /* exponents for CRT */
	u8 coefficient[MAX_RSA_PRIME_LEN]; /* CRT coefficient */
} R_RSA_PRIVATE_KEY;

s32 rsapublicfunc(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		  R_RSA_PUBLIC_KEY *publicKey);
s32 rsaprivatefunc(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		   R_RSA_PRIVATE_KEY *privateKey);
s32 RSAPublicDecrypt(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		     R_RSA_PUBLIC_KEY *publicKey);
s32 RSAPrivateEncrypt(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		      R_RSA_PRIVATE_KEY *privateKey);
s32 RSAPrivateDecrypt(u8 *output, u32 *outputLen, u8 *input, u32 inputLen,
		      R_RSA_PRIVATE_KEY *privateKey);
s32 sunxi_rsa_calc_by_software(u8 *n_addr, u32 n_len, u8 *e_addr, u32 e_len,
			       u8 *dst_addr, u32 dst_len, u8 *src_addr,
			       u32 src_len);

#endif
