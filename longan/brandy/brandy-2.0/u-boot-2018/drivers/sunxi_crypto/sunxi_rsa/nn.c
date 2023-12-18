/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include "rdef.h"
#include "nn.h"

/* Computes a * b, result stored in high and low. */
void dmult(NN_DIGIT a, NN_DIGIT b, NN_DIGIT *high, NN_DIGIT *low)
{
	NN_HALF_DIGIT al, ah, bl, bh;
	NN_DIGIT m1, m2, m, ml, mh, carry = 0;

	al = (NN_HALF_DIGIT)LOW_HALF(a);
	ah = (NN_HALF_DIGIT)HIGH_HALF(a);
	bl = (NN_HALF_DIGIT)LOW_HALF(b);
	bh = (NN_HALF_DIGIT)HIGH_HALF(b);

	*low  = (NN_DIGIT)al * bl;
	*high = (NN_DIGIT)ah * bh;

	m1 = (NN_DIGIT)al * bh;
	m2 = (NN_DIGIT)ah * bl;
	m  = m1 + m2;

	if (m < m1)
		carry = 1L << (NN_DIGIT_BITS / 2);

	ml = (m & MAX_NN_HALF_DIGIT) << (NN_DIGIT_BITS / 2);
	mh = m >> (NN_DIGIT_BITS / 2);

	*low += ml;
	if (*low < ml)
		carry++;

	*high += carry + mh;
}

NN_DIGIT subdigitmult(NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT c, NN_DIGIT *d, u32 digits)
{
	NN_DIGIT borrow, thigh, tlow;
	u32 i;

	borrow = 0;
	if (c != 0) {
		for (i = 0; i < digits; i++) {
			dmult(c, d[i], &thigh, &tlow);
			a[i] = b[i] - borrow;
			if (a[i] > (MAX_NN_DIGIT - borrow))
				borrow = 1;
			else
				borrow = 0;
			a[i] -= tlow;
			if (a[i] > (MAX_NN_DIGIT - tlow))
				borrow++;
			borrow += thigh;
		}
	}

	return (borrow);
}

/* Returns the significant length of a in bits, where a is a digit. */
u32 NN_DigitBits (NN_DIGIT a)
{
	u32 i;

	for (i = 0; i < NN_DIGIT_BITS; i++, a >>= 1) {
		if (a == 0)
			break;
	}

	return (i);
}

void NN_Decode(NN_DIGIT *a, u32 digits, u8 *b, u32 len)
{
	NN_DIGIT t;
	u32 i, u;
	s32 j;

	/* @##$ unsigned/signed bug fix added JSAK - Fri  31/05/96 18:09:11 */
	for (i = 0, j = len - 1; i < digits && j >= 0; i++) {
		t = 0;
		for (u = 0; j >= 0 && u < NN_DIGIT_BITS; j--, u += 8) {
			t |= ((NN_DIGIT)b[j]) << u;
		}
		a[i] = t;
	}

	for (; i < digits; i++)
		a[i] = 0;
}

/* Encodes b into character string a, where character string is ordered
   from most to least significant.
   Lengths: a[len], b[digits].
   Assumes NN_Bits (b, digits) <= 8 * len. (Otherwise most significant
   digits are truncated.)
 */
void NN_Encode (u8 *a, u32 len, NN_DIGIT *b, u32 digits)
{
	NN_DIGIT t;
	u32 i, u;
	s32 j;

	/* @##$ unsigned/signed bug fix added JSAK - Fri  31/05/96 18:09:11 */
	for (i = 0, j = len - 1; i < digits && j >= 0; i++) {
		t = b[i];
		for (u       = 0; j >= 0 && u < NN_DIGIT_BITS; j--, u += 8)
			a[j] = (u8)(t >> u);
	}

	for (; j >= 0; j--) {
		a[j] = 0;
	}
}

/* Assigns a = 0. */
void NN_AssignZero (NN_DIGIT *a, u32 digits)
{
	if (digits) {
		do {
			*a++ = 0;
		} while (--digits);
	}
}

/* Assigns a = 2^b.
   Lengths: a[digits].
   Requires b < digits * NN_DIGIT_BITS.
 */
void NN_Assign2Exp (NN_DIGIT *a, u32 b, u32 digits)
{
	NN_AssignZero(a, digits);

	if (b >= digits * NN_DIGIT_BITS)
		return;

	a[b / NN_DIGIT_BITS] = (NN_DIGIT)1 << (b % NN_DIGIT_BITS);
}

/* Computes a = b - c. Returns borrow.
   Lengths: a[digits], b[digits], c[digits].
 */
NN_DIGIT NN_Sub (NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 digits)
{
	NN_DIGIT temp, borrow = 0;

	if (digits) {
		do {
			/* Bug fix 16/10/95 - JSK, code below removed, caused bug with Sun Compiler SC4.
			 * if((temp = (*b++) - borrow) == MAX_NN_DIGIT)
			 *      temp = MAX_NN_DIGIT - *c++;
			 */

			temp = *b - borrow;
			b++;
			if (temp == MAX_NN_DIGIT) {
				temp = MAX_NN_DIGIT - *c;
				c++;
			} else { /* Patch to prevent bug for Sun CC */
				temp -= *c;
				if (temp > (MAX_NN_DIGIT - *c))
					borrow = 1;
				else
					borrow = 0;
				c++;
			}
			*a++ = temp;
		} while (--digits);
	}

	return (borrow);
}

/* Computes a = b * c.
   Lengths: a[2*digits], b[digits], c[digits].
   Assumes digits < MAX_NN_DIGITS.
*/
void NN_Mult (NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 digits)
{
	NN_DIGIT t[2 * MAX_NN_DIGITS];
	NN_DIGIT dhigh, dlow, carry;
	u32 bDigits, cDigits, i, j;

	NN_AssignZero(t, 2 * digits);
	bDigits = NN_Digits(b, digits);
	cDigits = NN_Digits(c, digits);

	for (i = 0; i < bDigits; i++) {
		carry = 0;
		if (*(b + i) != 0) {
			for (j = 0; j < cDigits; j++) {
				dmult(*(b + i), *(c + j), &dhigh, &dlow);
				*(t + (i + j)) = *(t + (i + j)) + carry;
				if (*(t + (i + j)) < carry)
					carry = 1;
				else
					carry = 0;
				*(t + (i + j)) += dlow;
				if (*(t + (i + j)) < dlow)
					carry++;
				carry += dhigh;
			}
		}
		*(t + (i + cDigits)) += carry;
	}

	NN_Assign(a, t, 2 * digits);
}

/* Computes a = b * 2^c (i.e., shifts left c bits), returning carry.
   Requires c < NN_DIGIT_BITS. */
NN_DIGIT NN_LShift (NN_DIGIT *a, NN_DIGIT *b, u32 c, u32 digits)
{
	NN_DIGIT temp, carry = 0;
	u32 t;

	if (c < NN_DIGIT_BITS) {
		if (digits) {
			t = NN_DIGIT_BITS - c;
			do {
				temp  = *b++;
				*a++  = (temp << c) | carry;
				carry = c ? (temp >> t) : 0;
			} while (--digits);
		}
	}

	return (carry);
}

/* Computes a = c div 2^c (i.e., shifts right c bits), returning carry.
   Requires: c < NN_DIGIT_BITS. */
NN_DIGIT NN_RShift (NN_DIGIT *a, NN_DIGIT *b, u32 c, u32 digits)
{
	NN_DIGIT temp, carry = 0;
	u32 t;

	if (c < NN_DIGIT_BITS) {
		if (digits) {
			t = NN_DIGIT_BITS - c;
			do {
				digits--;
				temp	  = *(b + digits);
				*(a + digits) = (temp >> c) | carry;
				carry	 = c ? (temp << t) : 0;
			} while (digits);
		}
	}

	return (carry);
}

/* Computes a = c div d and b = c mod d.
   Lengths: a[cDigits], b[dDigits], c[cDigits], d[dDigits].
   Assumes d > 0, cDigits < 2 * MAX_NN_DIGITS, dDigits < MAX_NN_DIGITS.
*/
void NN_Div (NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 cDigits, NN_DIGIT *d, u32 dDigits)
{
	NN_DIGIT ai, cc[2 * MAX_NN_DIGITS + 1], dd[MAX_NN_DIGITS], s;
	NN_DIGIT t[2], u, v, *ccptr;
	NN_HALF_DIGIT aHigh, aLow, cHigh, cLow;
	s32 i;
	u32 ddDigits, shift;

	ddDigits = NN_Digits(d, dDigits);
	if (ddDigits == 0)
		return;

	shift = NN_DIGIT_BITS - NN_DigitBits(d[ddDigits - 1]);
	NN_AssignZero(cc, ddDigits);
	cc[cDigits] = NN_LShift(cc, c, shift, cDigits);
	NN_LShift(dd, d, shift, ddDigits);
	s = dd[ddDigits - 1];

	NN_AssignZero(a, cDigits);

	for (i = cDigits - ddDigits; i >= 0; i--) {
		if (s == MAX_NN_DIGIT)
			ai = cc[i + ddDigits];
		else {
			ccptr = &cc[i + ddDigits - 1];

			s++;
			cHigh = (NN_HALF_DIGIT)HIGH_HALF(s);
			cLow  = (NN_HALF_DIGIT)LOW_HALF(s);

			*t       = *ccptr;
			*(t + 1) = *(ccptr + 1);

			if (cHigh == MAX_NN_HALF_DIGIT)
				aHigh = (NN_HALF_DIGIT)HIGH_HALF(*(t + 1));
			else
				aHigh = (NN_HALF_DIGIT)(*(t + 1) / (cHigh + 1));
			u	     = (NN_DIGIT)aHigh * (NN_DIGIT)cLow;
			v	     = (NN_DIGIT)aHigh * (NN_DIGIT)cHigh;
			*t -= TO_HIGH_HALF(u);
			if (*t > (MAX_NN_DIGIT - TO_HIGH_HALF(u)))
				t[1]--;
			*(t + 1) -= HIGH_HALF(u);
			*(t + 1) -= v;

			while ((*(t + 1) > cHigh) ||
			       ((*(t + 1) == cHigh) &&
				(*t >= TO_HIGH_HALF(cLow)))) {
				*t -= TO_HIGH_HALF(cLow);
				if (*t > MAX_NN_DIGIT - TO_HIGH_HALF(cLow))
					t[1]--;
				*(t + 1) -= cHigh;
				aHigh++;
			}

			if (cHigh == MAX_NN_HALF_DIGIT)
				aLow = (NN_HALF_DIGIT)LOW_HALF(*(t + 1));
			else
				aLow = (NN_HALF_DIGIT)((TO_HIGH_HALF(*(t + 1)) +
							HIGH_HALF(*t)) /
						       (cHigh + 1));
			u = (NN_DIGIT)aLow * (NN_DIGIT)cLow;
			v = (NN_DIGIT)aLow * (NN_DIGIT)cHigh;
			*t -= u;
			if (*t > (MAX_NN_DIGIT - u))
				t[1]--;
			*t -= TO_HIGH_HALF(v);
			if (*t > (MAX_NN_DIGIT - TO_HIGH_HALF(v)))
				t[1]--;
			*(t + 1) -= HIGH_HALF(v);

			while ((*(t + 1) > 0) || ((*(t + 1) == 0) && *t >= s)) {
				*t -= s;
				if (*t > (MAX_NN_DIGIT - s))
					t[1]--;
				aLow++;
			}

			ai = TO_HIGH_HALF(aHigh) + aLow;
			s--;
		}

		cc[i + ddDigits] -=
			subdigitmult(&cc[i], &cc[i], ai, dd, ddDigits);

		while (cc[i + ddDigits] ||
		       (NN_Cmp(&cc[i], dd, ddDigits) >= 0)) {
			ai++;
			cc[i + ddDigits] -=
				NN_Sub(&cc[i], &cc[i], dd, ddDigits);
		}

		a[i] = ai;
	}

	NN_AssignZero(b, dDigits);
	NN_RShift(b, cc, shift, ddDigits);
}


/* Computes a = b mod c.
   Lengths: a[cDigits], b[bDigits], c[cDigits].
   Assumes c > 0, bDigits < 2 * MAX_NN_DIGITS, cDigits < MAX_NN_DIGITS.
*/
void NN_Mod (NN_DIGIT *a, NN_DIGIT *b, u32 bDigits, NN_DIGIT *c, u32 cDigits)
{
	NN_DIGIT t[2 * MAX_NN_DIGITS];

	NN_Div(t, a, b, bDigits, c, cDigits);
}

/* Computes a = b * c mod d.
   Lengths: a[digits], b[digits], c[digits], d[digits].
   Assumes d > 0, digits < MAX_NN_DIGITS.
 */
void NN_ModMult (NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, NN_DIGIT *d, u32 digits)
{
	NN_DIGIT t[2 * MAX_NN_DIGITS];

	NN_Mult(t, b, c, digits);
	NN_Mod(a, t, 2 * digits, d, digits);
}

/* Computes a = b^c mod d.

	 Lengths: a[dDigits], b[dDigits], c[cDigits], d[dDigits].
	 Assumes d > 0, cDigits > 0, dDigits < MAX_NN_DIGITS.
 */
void NN_ModExp (NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 cDigits, NN_DIGIT *d, u32 dDigits)
{
	NN_DIGIT bPower[3][MAX_NN_DIGITS], ci, t[MAX_NN_DIGITS];
	s32 i;
	u32 ciBits, j, s;

	/* Store b, b^2 mod d, and b^3 mod d. */
	NN_Assign(bPower[0], b, dDigits);
	NN_ModMult(bPower[1], bPower[0], b, d, dDigits);
	NN_ModMult(bPower[2], bPower[1], b, d, dDigits);
	NN_ASSIGN_DIGIT(t, 1, dDigits);

	cDigits = NN_Digits(c, cDigits);
	for (i = cDigits - 1; i >= 0; i--) {
		ci     = c[i];
		ciBits = NN_DIGIT_BITS;

		/* Scan past leading zero bits of most significant digit. */
		if (i == (s32)(cDigits - 1)) {
			while (!DIGIT_2MSB(ci)) {
				ci <<= 2;
				ciBits -= 2;
			}
		}

		for (j = 0; j < ciBits; j += 2, ci <<= 2) {
			/* Compute t = t^4 * b^s mod d, where s = two MSB's of ci. */
			NN_ModMult(t, t, t, d, dDigits);
			NN_ModMult(t, t, t, d, dDigits);
			s = DIGIT_2MSB(ci);
			if (s != 0)
				NN_ModMult(t, t, bPower[s - 1], d, dDigits);
		}
	}

	NN_Assign(a, t, dDigits);
}

/* Compute a = 1/b mod c, assuming inverse exists.
   Lengths: a[digits], b[digits], c[digits].
   Assumes gcd (b, c) = 1, digits < MAX_NN_DIGITS.
 */
void NN_ModInv (NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 digits)
{
	NN_DIGIT q[MAX_NN_DIGITS], t1[MAX_NN_DIGITS], t3[MAX_NN_DIGITS],
		u1[MAX_NN_DIGITS], u3[MAX_NN_DIGITS], v1[MAX_NN_DIGITS],
		v3[MAX_NN_DIGITS], w[2 * MAX_NN_DIGITS];
	s32 u1Sign;

	/* Apply extended Euclidean algorithm, modified to avoid negative
	   numbers.	*/
	NN_ASSIGN_DIGIT(u1, 1, digits);
	NN_AssignZero(v1, digits);
	NN_Assign(u3, b, digits);
	NN_Assign(v3, c, digits);
	u1Sign = 1;

	while (!NN_Zero(v3, digits)) {
		NN_Div(q, t3, u3, digits, v3, digits);
		NN_Mult(w, q, v1, digits);
		NN_Add(t1, u1, w, digits);
		NN_Assign(u1, v1, digits);
		NN_Assign(v1, t1, digits);
		NN_Assign(u3, v3, digits);
		NN_Assign(v3, t3, digits);
		u1Sign = -u1Sign;
	}

	/* Negate result if sign is negative. */
	if (u1Sign < 0)
		NN_Sub(a, c, u1, digits);
	else
		NN_Assign(a, u1, digits);
}

/* Computes a = gcd(b, c).
   Assumes b > c, digits < MAX_NN_DIGITS.
*/

#define iplus1  (i == 2 ? 0 : i + 1)      /* used by Euclid algorithms */
#define iminus1 (i == 0 ? 2 : i - 1)      /* used by Euclid algorithms */
#define g(i)    (&(t[i][0]))

void NN_Gcd(NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 digits)
{
	s16 i;
	NN_DIGIT t[3][MAX_NN_DIGITS];

	NN_Assign(g(0), c, digits);
	NN_Assign(g(1), b, digits);

	i = 1;

	while (!NN_Zero(g(i), digits)) {
		NN_Mod(g(iplus1), g(iminus1), digits, g(i), digits);
		i = iplus1;
	}

	NN_Assign(a, g(iminus1), digits);
}

/* Returns the significant length of a in bits.
   Lengths: a[digits]. */

u32 NN_Bits (NN_DIGIT *a, u32 digits)
{
	digits = NN_Digits(a, digits);
	if (digits == 0)
		return (0);

	return ((digits - 1) * NN_DIGIT_BITS + NN_DigitBits(a[digits - 1]));
}

/* Returns sign of a - b. */
s32 NN_Cmp(NN_DIGIT *a, NN_DIGIT *b, u32 digits)
{
	if (digits) {
		do {
			digits--;
			if (*(a + digits) > *(b + digits))
				return (1);
			if (*(a + digits) < *(b + digits))
				return (-1);
		} while (digits);
	}

	return (0);
}

/* Returns nonzero iff a is zero. */
s32 NN_Zero (NN_DIGIT *a, u32 digits)
{
	if (digits) {
		do {
			if (*a++)
				return (0);
		} while (--digits);
	}

	return (1);
}

/* Assigns a = b. */
void NN_Assign (NN_DIGIT *a, NN_DIGIT *b, u32 digits)
{
	if (digits) {
		do {
			*a++ = *b++;
		} while (--digits);
	}
}

/* Returns the significant length of a in digits. */
u32 NN_Digits (NN_DIGIT *a, u32 digits)
{
	if (digits) {
		digits--;
		do {
			if (*(a + digits))
				break;
		} while (digits--);

		return (digits + 1);
	}

	return (digits);
}

/* Computes a = b + c. Returns carry.
   Lengths: a[digits], b[digits], c[digits].
 */
NN_DIGIT NN_Add(NN_DIGIT *a, NN_DIGIT *b, NN_DIGIT *c, u32 digits)
{
	NN_DIGIT temp, carry = 0;

	if (digits) {
		do {
			temp = (*b++) + carry;
			if (temp < carry) {
				temp = *c++;
			} else { /* Patch to prevent bug for Sun CC */
				temp += *c;
				if (temp < *c)
					carry = 1;
				else
					carry = 0;
				c++;
			}
			*a++ = temp;
		} while (--digits);
	}

	return (carry);
}
