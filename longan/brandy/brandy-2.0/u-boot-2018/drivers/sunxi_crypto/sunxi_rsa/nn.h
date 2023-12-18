/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _NN_H_
#define _NN_H_

/* Type definitions. */
typedef unsigned int NN_DIGIT;
typedef unsigned short NN_HALF_DIGIT;

/* Constants.
	Note: MAX_NN_DIGITS is long enough to hold any RSA modulus, plus
	one more digit as required by R_GeneratePEMKeys (for n and phiN,
	whose lengths must be even). All natural numbers have at most
	MAX_NN_DIGITS digits, except for double-length intermediate values
	in NN_Mult (t), NN_ModMult (t), NN_ModInv (w), and NN_Div (c).
*/

/* Length of digit in bits */
#define NN_DIGIT_BITS      32
#define NN_HALF_DIGIT_BITS 16
/* Length of digit in bytes */
#define NN_DIGIT_LEN       (NN_DIGIT_BITS / 8)
/* Maximum length in digits */
#define MAX_NN_DIGITS      ((MAX_RSA_MODULUS_LEN + NN_DIGIT_LEN - 1) / NN_DIGIT_LEN + 1)
/* Maximum digits */
#define MAX_NN_DIGIT       0xffffffff
#define MAX_NN_HALF_DIGIT  0xffff

#define NN_LT   -1
#define NN_EQ   0
#define NN_GT   1

/* Macros. */

#define LOW_HALF(x)     ((x) & MAX_NN_HALF_DIGIT)
#define HIGH_HALF(x)    (((x) >> NN_HALF_DIGIT_BITS) & MAX_NN_HALF_DIGIT)
#define TO_HIGH_HALF(x) (((NN_DIGIT)(x)) << NN_HALF_DIGIT_BITS)
#define DIGIT_MSB(x)    (u32)(((x) >> (NN_DIGIT_BITS - 1)) & 1)
#define DIGIT_2MSB(x)   (u32)(((x) >> (NN_DIGIT_BITS - 2)) & 3)

void NN_Decode(NN_DIGIT *, u32, u8 *, u32);
void NN_Encode(u8 *, u32, NN_DIGIT *, u32);

void NN_Assign(NN_DIGIT *, NN_DIGIT *, u32);
void NN_AssignZero(NN_DIGIT *, u32);
void NN_Assign2Exp(NN_DIGIT *, u32, u32);

NN_DIGIT NN_Add(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32);
NN_DIGIT NN_Sub(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32);
void NN_Mult(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32);
void NN_Div(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32, NN_DIGIT *, u32);
NN_DIGIT NN_LShift(NN_DIGIT *, NN_DIGIT *, u32, u32);
NN_DIGIT NN_RShift(NN_DIGIT *, NN_DIGIT *, u32, u32);
NN_DIGIT NN_LRotate(NN_DIGIT *, NN_DIGIT *, u32, u32);

void NN_Mod(NN_DIGIT *, NN_DIGIT *, u32, NN_DIGIT *, u32);
void NN_ModMult(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32);
void NN_ModExp(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32, NN_DIGIT *, u32);
void NN_ModInv(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32);
void NN_Gcd(NN_DIGIT *, NN_DIGIT *, NN_DIGIT *, u32);

s32 NN_Cmp(NN_DIGIT *, NN_DIGIT *, u32);
s32 NN_Zero(NN_DIGIT *, u32);
u32 NN_Bits(NN_DIGIT *, u32);
u32 NN_Digits(NN_DIGIT *, u32);

#define NN_ASSIGN_DIGIT(a, b, digits) {NN_AssignZero (a, digits); a[0] = b; }
#define NN_EQUAL(a, b, digits) (!NN_Cmp (a, b, digits))
#define NN_EVEN(a, digits) (((digits) == 0) || !(a[0] & 1))

#endif /* _NN_H_ */
