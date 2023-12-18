/**************************************************************************
 * MODULE NAME:    lmp_ecc.c
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    LMP Handling for ECC.
 * AUTHOR:         Gary Fleming
 * DATE:           02 Feb 2009
 *
 * SOURCE CONTROL: $Id: lmp_ecc.c,v 1.24 2015/02/20 15:34:57 garyf Exp $
 *
 *
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2009 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 * NOTES:
 * This module encompasses the protocol exchange to support Secure Simple Pairing
 *
 * This module encompasses the protocol only and depends on the cryptographic functions
 * which are defined in lmp_ssp_engine.c and lmp_ecc.c
 *
 **************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lmp_ecc.h"

///#if (PRH_BS_CFG_SYS_LMP_SECURE_SIMPLE_PAIRING_SUPPORTED==1)

// Turn on Below for 'best' ARM optimisation of this file.
//#pragma Otime
//#pragma O2

#define JACOBIAN_METHOD //AFFINE_METHOD

/*******************************************************************************************
 * The Base Point of the ECC curve.
 * We obtain our public key by multiplying our secret key by this point
 *******************************************************************************************/

 const uint8_t BasePoint_x[24] = { 0x18,0x8d,0xa8,0x0e,0xb0,0x30,0x90,0xf6,0x7c,0xbf,0x20,0xeb,
						   0x43,0xa1,0x88,0x00,0xf4,0xff,0x0a,0xfd,0x82,0xff,0x10,0x12};

 const uint8_t BasePoint_y[24] = { 0x07,0x19,0x2b,0x95,0xff,0xc8,0xda,0x78,0x63,0x10,0x11,0xed,
						   0x6b,0x24,0xcd,0xd5,0x73,0xf9,0x77,0xa1,0x1e,0x79,0x48,0x11};

/*******************************************************************************************
 * Maximum secret key allowed.
 *******************************************************************************************/

 const uint8_t maxSecretKey[24] = { 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                   0xcc,0xef,0x7c,0x1b,0x0a,0x35,0xe4,0xd8,0xda,0x69,0x14,0x18};

#ifdef _MSC_VER
#if (_MSC_VER==1200)
#define _MSVC_6
#endif
#endif

#ifdef _MSVC_6
 typedef __int64 u64;
#define _LL(x) x
#else
 typedef unsigned long long int u64;
#define _LL(x) x##ll
#endif

/*******************************************************************************************
 * The prime number P for the ECC curve.
 * We represent this in two formats.
 *******************************************************************************************/

 const bigHex bigHexP = {{ 0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE,0xFFFFFFFF,0xFFFFFFFF},
					6,0};

 const veryBigHex veryBigHexP = {{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
      0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE,0xFFFFFFFF,0xFFFFFFFF},
					6,0};

 /*******************************************************************************************
  * The Jacobian point of infinity.
  *    X = 0, Y = 1, Z = 0
  *******************************************************************************************/

 const  ECC_Jacobian_Point LMecc_Jacobian_InfinityPoint = { {{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000 },0x00,0x00},
 														   {{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000001 },0x01,0x00},
 														   {{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000 },0x00,0x00}};


#define MAX_DIGITS 56
#define ELEMENTS_BIG_HEX 7 // MAX_OCTETS/4
#define HIGHEST_INDEX_BIG_HEX  6// (MAX_OCTETS/4 )- 1
#define ISBIGHEXEVEN(x) (!((x.num[HIGHEST_INDEX_BIG_HEX]) & 0x01))
#define GETMOSTSIGNIGICANTU32(tmpHexA)  tmpHexA->num[ELEMENTS_BIG_HEX-tmpHexA->len]

static bigHex PrivateKey;
static ECC_Point PublicKey;
//static ECC_Point ResultPoint;
static uint32_t ECC_Point_Mul_Word = 0;

static ECC_Jacobian_Point LMecc_Jacobian_PointP;
ECC_Jacobian_Point LMecc_Jacobian_PointQ;
static ECC_Jacobian_Point LMecc_Jacobian_PointR;
ECC_Point LMecc_PointQ;

static bigHex LMecc_Pk;

static uint8_t public_key_x[24];
static uint8_t public_key_y[24];
/*******************************************************************************************
 * 3 Simple operators on bigHex numbers. These operators do not consider the GF, and thus the
 * result can be outside the Prime Field. These are used to construct more advanced operators
 * which consider the GF and reflect the result back into the prime field.
 *******************************************************************************************/

void AddBigHex(const bigHex *bigHexA,const bigHex *bigHexB, bigHex *BigHexResult);
void SubtractBigHex( const bigHex *bigHexA, const bigHex *bigHexB, bigHex *BigHexResult);
void MultiplyBigHex( const bigHex *bigHexA, const bigHex *bigHexB, veryBigHex *BigHexResult);
void bigHexInversion( bigHex* bigHexA, bigHex* pResult);
void Add2SelfBigHex( bigHex *bigHexA,const bigHex *bigHexB);
void SubtractFromSelfBigHexSign( bigHex *bigHexA, const bigHex *bigHexB);

void initBigNumber(bigHex *BigHexResult);
void setBigNumberLength( bigHex *BigHexResult);
void setVeryBigNumberLength(veryBigHex *BigHexResult);

void copyBigHex(const bigHex *source,bigHex *destination);
void copyVeryBigHex(const veryBigHex *source,veryBigHex *destination);

void shiftLeftOneArrayElement(bigHex *input);
void MultiplyBigHexModP(const bigHex *bigHexA,const bigHex *bigHexB, bigHex *BigHexResult);

void divideByTwo(bigHex* A);

int isbigHexEven(const bigHex* A);
int isGreaterThan(const bigHex *bigHexA,const bigHex *bigHexB);
int isGreaterThanOrEqual(const bigHex *bigHexA,const bigHex *bigHexB);
int isVeryBigHexGreaterThanOrEqual(const veryBigHex *bigHexA,const veryBigHex *bigHexB);

void GF_Setup_Jacobian_Infinity_Point(ECC_Jacobian_Point *infinity);
void GF_Affine_To_Jacobian_Point_Copy(const ECC_Point *source,ECC_Jacobian_Point *destination);

void MultiplyByU32ModP(const uint32_t N,bigHex* result);
void MultiplyByU32ModPInv(const uint32_t N,bigHex* result);
void specialModP(bigHex *tmpHexA);
void AddPdiv2(bigHex *bigHexA);
void AddP(bigHex *bigHexA);
void SubtractFromSelfBigHex(bigHex *bigHexA,const bigHex *bigHexB);
// Galois Field Operands
void GF_Point_Addition(const ECC_Point* PointP,const ECC_Point* PointQ, ECC_Point* ResultPoint);
int GF_Jacobian_Point_Double(const ECC_Jacobian_Point* pPointP, ECC_Jacobian_Point* pResultPoint);
void LMecc_CB_ECC_Point_Multiplication_Complete(void* p_link);
void ECC_Point_Multiplication_uint32(void* p_link);
void ECC_Point_Multiplication_uint8(void* p_link,uint8_t blocking);
void ECC_Point_Multiplication_uint8_non_blocking(void* p_link );


int LMecc_isValidSecretKey(uint8_t* secretKey)
{
	// First check that the secret key is not all zeros.
	//
	int i;

	for(i=0;i<24;i++)
	{
		if (secretKey[i] != 0)
			break;
	}

	if ((i==24) && (secretKey[23] == 0))
		return 0;

	for(i = 0; i < 24; i++)
	{
		if (secretKey[i] > maxSecretKey[i])
			return 0;
		else if (secretKey[i] < maxSecretKey[i])
			return 1;
	}
    return 1;
}

/******************************************************************************
 * Section of functions for BASIC big number handling
 *   These do not consider the Finite Field or ECC
 *
 *  Mod P is not performed
 *  Sign is not considered -- all inputs and results  are assumed to be positive
 *
 *  AddBigHex            - This adds two 15*u16 positive numbers.
 *  AddVeryBigHex        - This adds two 30*u16 positive numbers.
 *
 *  SubtractBigHex       - This subtracts two 15*u16 positive numbers
 *                         To ensure a +ive result we assume A >= B
 *  SubtractVeryBigHex   - This subtracts two 30*u16 positive numbers
 *                         To ensure a +ive result we assume A >= B
 *  MultiplyBigHex       - This multiplies two 15*u16 positive numbers to produce
 *                         a 30*u16 positive result
 *
 ******************************************************************************/

//
//
// Inputs - Format of the inputs that they begin with the LEAST significant bytes in the 0 entry of the array.
//
void AddBigHex( const bigHex *bigHexA, const bigHex *bigHexB, bigHex *BigHexResult)
{
	u64 tmp;
	uint32_t carry = 0;
	int32_t i;
	const uint32_t *numA = bigHexA->num;
	const uint32_t *numB = bigHexB->num;
	uint32_t *result = BigHexResult->num;

	for (i=(ELEMENTS_BIG_HEX-1);i >= 0; i--)
	{
		tmp = (u64)((u64)(numA[i]) + (u64)(numB[i]) + carry);
		if(tmp & _LL(0x100000000))
			carry = 0x01;
		else
			carry = 0x00;

		result[i] = (tmp & 0xFFFFFFFF);
	}
	setBigNumberLength(BigHexResult);
    BigHexResult->sign = 0; //XXXXX
}

void AddBigHexModP( const bigHex *bigHexA, const bigHex *bigHexB, bigHex *BigHexResult)
{
	AddBigHex(bigHexA,bigHexB,BigHexResult);

	if (BigHexResult->sign == 0x00)   // Positive Number
	{
		if(isGreaterThanOrEqual(BigHexResult,&bigHexP))
	    {
			//SubtractFromSelfBigHexSign(BigHexResult,&bigHexP);
			SubtractFromSelfBigHex(BigHexResult, &bigHexP);
		}
	}
	else
	{
		AddP(BigHexResult);
	}
}
// A = A + B
void Add2SelfBigHex( bigHex *bigHexA,const bigHex *bigHexB)
{
	u64 tmp;
	uint32_t carry = 0;
	int32_t i;
	uint32_t *numA = bigHexA->num;
	const uint32_t *numB = bigHexB->num;

	for (i=(ELEMENTS_BIG_HEX-1);i >= 0; i--)
	{
		tmp = (u64)((u64)(numA[i]) + (u64)(numB[i]) + carry);
		if(tmp & _LL(0x100000000))
			carry = 0x01;
		else
			carry = 0x00;

		numA[i] = (tmp & 0xFFFFFFFF);
	}
	setBigNumberLength(bigHexA);
}

void SubtractBigHex(const bigHex *bigHexA, const bigHex *bigHexB, bigHex *BigHexResult)
{
    // Due to the nature of the GF we have to assume both inputs are +ive.
	uint32_t borrow=0;
	int32_t i;
	const uint32_t* pBigNum = bigHexA->num;
	const uint32_t* pSmallNum = bigHexB->num;
	uint32_t* pResult = BigHexResult->num;

	for (i=(ELEMENTS_BIG_HEX-1);i >= 0; i--)
	{
 		if (((u64)((u64)(pSmallNum[i])+(u64)borrow)) > (pBigNum[i]))
		{
			pResult[i] = (((u64)(pBigNum[i] + _LL(0x100000000)))- (pSmallNum[i]+borrow)) & 0xFFFFFFFF;
			borrow = 0x01;
		}
		else
		{
			pResult[i] = ((pBigNum[i])- (pSmallNum[i]+borrow)) & 0xFFFFFFFF;
			borrow = 0x00;
		}
	}
    setBigNumberLength(BigHexResult);
}

// A = A - B

void SubtractFromSelfBigHex(bigHex *bigHexA,const bigHex *bigHexB)
{
    // Due to the nature of the GF we have to assume both inputs are +ive.
	uint32_t borrow=0;
	int32_t i;
	uint32_t* pBigNum = bigHexA->num;
	const uint32_t* pSmallNum = bigHexB->num;

	for (i=(ELEMENTS_BIG_HEX-1);i >= 0; i--)
	{
 		if (((u64)((u64)(pSmallNum[i])+(u64)borrow)) > (pBigNum[i]))
		{
			pBigNum[i]  = (((u64)(pBigNum[i] + _LL(0x100000000)))- (pSmallNum[i]+borrow)) & 0xFFFFFFFF;
			borrow = 0x01;
		}
		else
		{
			pBigNum[i] = ((pBigNum[i])- (pSmallNum[i]+borrow)) & 0xFFFFFFFF;
			borrow = 0x00;
		}
	}
    setBigNumberLength(bigHexA);
}

/**********************************************************************************
 * This section extends of the basic functions provided previously to support
 * sign. We concentrate on the Add and Subtract functions for both 15*u16 and 30*u16
 * numbers.
 *
 * AddBigHexSign         - Adds two 15*u16 numbers considering the sign.
 * SubtractBigHexSign    - Subtracts two 15*u16 numbers considering the sign
 * AddVeryBigHexSign
 * SubtractVeryBigHexSign
 * MultiplyBigHexSign    - Multiplies two 15*u16 numbers considering the sign.
 **********************************************************************************/

// AddP is only invoked when the sign of A is -ive
// A is always less than P
//
void AddP( bigHex *bigHexA)
{
	bigHex BigHexResult;
        memset(&BigHexResult,0,sizeof(BigHexResult));
	SubtractBigHex(&bigHexP, bigHexA,&BigHexResult);
	copyBigHex(&BigHexResult,bigHexA);
	bigHexA->sign = 0;
}

void AddPdiv2( bigHex *bigHexA)
{
	//
	uint32_t *numA = bigHexA->num;
	const uint32_t *numB = bigHexP.num;

	if (bigHexA->sign == 0)
	{
		u64 tmp;
		uint32_t carry;
		int32_t i;

		carry = 0;

		for (i=(ELEMENTS_BIG_HEX-1);i >= 0; i--)
		{
			tmp = (u64)((u64)(numA[i]) + (u64)(numB[i]) + carry);
			if(tmp & _LL(0x100000000))
				carry = 0x01;
			else
				carry = 0x00;

			numA[i] = (tmp & 0xFFFFFFFF);
		}
	}
	else // if (bigHexA->sign == 1)
	{
		if ( isGreaterThan(bigHexA, &bigHexP))
		{

			// Due to the nature of the GF we have to assume both inputs are +ive.
			uint32_t borrow=0;
			int32_t i;

			for (i=(ELEMENTS_BIG_HEX-1);i >= 0; i--)
			{
				if (((u64)((u64)(numB[i])+(u64)borrow)) > (numA[i]))
				{
					numA[i]  = (((u64)(numA[i] + _LL(0x100000000)))- (numB[i]+borrow)) & 0xFFFFFFFF;
					borrow = 0x01;
				}
				else
				{
					numA[i] = ((numA[i])- (numB[i]+borrow)) & 0xFFFFFFFF;
					borrow = 0x00;
				}
			}
		}
		else
		{
			bigHex BigHexResult;
            memset(&BigHexResult,0,sizeof(BigHexResult));
			SubtractBigHex(&bigHexP,bigHexA,&BigHexResult);
			copyBigHex(&BigHexResult,bigHexA);
			bigHexA->sign = 0;
		}
	}
	// divide by 2
	// divideByTwo(bigHexA);
	{
		uint32_t rem=0;
		u64 u64val;
		uint32_t i;

		for(i=0; i < (ELEMENTS_BIG_HEX); i++)
		{
			u64val = (u64)((u64)(numA[i]) + ((u64)rem * _LL(0x100000000)));

			numA[i] = (uint32_t)(u64val>>1);
			rem = (u64)(u64val - ((u64val>>1)<<1));
		}

		// We need to re-calculate the length.
		setBigNumberLength(bigHexA);
	}
}
void SubtractFromSelfBigHexSign( bigHex *bigHexA, const bigHex *bigHexB)
{
	// This function uses use the basis AddBigHex and SubtractBigHex to implement a full
	// Addition which considers sign and mod.
	//
	if (bigHexA->sign == 0)
	{
		if (bigHexB->sign == 0)
		{
			if ( isGreaterThanOrEqual(bigHexA,bigHexB))
			{
				SubtractFromSelfBigHex(bigHexA, bigHexB);
			}
			else
			{
				bigHex BigHexResult;
                memset(&BigHexResult,0,sizeof(BigHexResult));
				SubtractBigHex(bigHexB, bigHexA,&BigHexResult);
				copyBigHex(&BigHexResult,bigHexA);
				bigHexA->sign = 1;
			}
		}
		else
		{
			// 3/  if A is + and B is -   C =   A+B  Mod
			Add2SelfBigHex(bigHexA, bigHexB);
		}
	}
	else  // if (bigHexA->sign == 1)
	{
		if (bigHexB->sign == 0)
		{
			Add2SelfBigHex(bigHexA, bigHexB);
		}
		else
		{
			if ( isGreaterThanOrEqual(bigHexB,bigHexA))
			{
				bigHex BigHexResult;
                memset(&BigHexResult,0,sizeof(BigHexResult));
				SubtractBigHex(bigHexB, bigHexA,&BigHexResult);
				copyBigHex(&BigHexResult,bigHexA);
				bigHexA->sign = 0;
			}
			else
			{
				SubtractFromSelfBigHex(bigHexA, bigHexB);
			}
		}
	}
}
/*****************************************************************************
 * Following functions further extend on the basic functions to include Mod P.
 *
 * AddBigHexMod  -- This function takes A and B which can be signed and output
 *                  a result C which is less than P.
 *                  It calls AddBigHexSign and then performs Mod P on the output.
 *                  If the output is -ive it recursively adds P until we get a +ive
 *                  number which is less than P.
 *                  If the output is +ive it recursively subtracts P until we get
 *                  a +number which is less than P.
 *
 * SubtractBigHexMod  -- This function takes A and B which can be signed and output
 *                  a result C which is less than P.
 *                  It calls SubtractBigHexSign and then performs Mod P on the output.
 *                  If the output is -ive it recursively add P until we get a +ive
 *                  number which is less than P.
 *                  If the output is +ive it recursively subtracts P until we get
 *                  a +number which is less than P.
 *
 * MultiplyBigHexMod
 ****************************************************************************/
void SubtractBigHexMod(const bigHex *bigHexA, const bigHex *bigHexB, bigHex *BigHexResult)
{
	if (bigHexA->sign == 0)
	{
		if (bigHexB->sign == 0)
		{
			if ( isGreaterThanOrEqual(bigHexA,bigHexB))
			{
				SubtractBigHex(bigHexA, bigHexB,BigHexResult);
				BigHexResult->sign = 0;
			}
			else
			{
				SubtractBigHex(bigHexB, bigHexA,BigHexResult);
				BigHexResult->sign = 1;
			}
		}
		else
		{
			// 3/  if A is + and B is -   C =   A+B  Mod
			AddBigHex(bigHexA, bigHexB,BigHexResult);
			BigHexResult->sign = 0;
		}
	}
	else  // if (bigHexA->sign == 1)
	{
		if (bigHexB->sign == 0)
		{
			AddBigHex(bigHexA, bigHexB,BigHexResult);
			BigHexResult->sign = 1;
		}
		else
		{
			if ( isGreaterThanOrEqual(bigHexB,bigHexA))
			{
				SubtractBigHex(bigHexB, bigHexA,BigHexResult);
				BigHexResult->sign = 0;
			}
			else
			{
				SubtractBigHex(bigHexA, bigHexB,BigHexResult);
				BigHexResult->sign = 1;
			}
		}
	}

	if (BigHexResult->sign == 0x00)   // Positive Number
	{
		if(isGreaterThanOrEqual(BigHexResult,&bigHexP))
	    {
			//SubtractFromSelfBigHexSign(BigHexResult,&bigHexP);
			SubtractFromSelfBigHex(BigHexResult, &bigHexP);
		}
	}
	else
	{
		AddP(BigHexResult);
	}
}

// From current usage we can safely assume both inputs are positive.

void SubtractBigHexUint32(const bigHex *bigHexA, const uint32_t numB, bigHex *BigHexResult)
{
	initBigNumber(BigHexResult);

	if (bigHexA->num[HIGHEST_INDEX_BIG_HEX] >= numB)
	{
		copyBigHex(bigHexA,BigHexResult);
		BigHexResult->num[ELEMENTS_BIG_HEX-1] = bigHexA->num[ELEMENTS_BIG_HEX-1]-numB;
	}
	else
	{
		bigHex N;

		initBigNumber(&N);

		N.len = 1;
		N.num[HIGHEST_INDEX_BIG_HEX] = numB;
		SubtractBigHexMod(bigHexA,&N,BigHexResult);
	}
}


void MultiplyBigHexModP(const bigHex *bigHexA, const bigHex *bigHexB, bigHex *BigHexResult)
{
	veryBigHex tmpResult;
	{
		int32_t k;
		const uint32_t *numA = bigHexA->num;
		const uint32_t *numB = bigHexB->num;
		uint32_t *result = tmpResult.num;

		memset(tmpResult.num,0, sizeof(tmpResult.num) );
		tmpResult.len = 0;
		tmpResult.sign = 0;
		//
		// Below Optimisations seem to degrade performance on Windows by a factor of 2.
		//
		for(k=(ELEMENTS_BIG_HEX-1);k >= 0;k--)
		{
			uint32_t mcarry = 0;
			int32_t j = 0;

			// The inner loop multiplies each byte of HexB by a single byte of
			// HexA
			for(j=(ELEMENTS_BIG_HEX-1);j >=0;j--)
			{
				u64 val;

				val = (((u64)(numA[k]) ) * ((u64)(numB[j]))) + result[j+k+1] + mcarry;
				result[j+k+1] = (val & 0xFFFFFFFF);
				mcarry = (uint32_t)( val >> 32);
			}
		}
		setVeryBigNumberLength(&tmpResult);
		tmpResult.sign = (bigHexA->sign != bigHexB->sign);
	}

	{
		uint32_t i=0;
		bigHex tmpResult2;

		while ((tmpResult.num[i] == 0x00) && (i<(ELEMENTS_BIG_HEX)))
			i++;

		// Take the next 13 elements
		//
		tmpResult2.sign = tmpResult.sign;

		if (isVeryBigHexGreaterThanOrEqual(&tmpResult,&veryBigHexP))
		{
			while(tmpResult.num[i] == 0x00)
				i++;
		}
		memcpy(tmpResult2.num, tmpResult.num + i,
		sizeof(tmpResult.num)-sizeof(tmpResult.num[0])*i > sizeof(tmpResult2.num) ? sizeof(tmpResult2.num) : sizeof(tmpResult.num)-sizeof(tmpResult.num[0])*i);

		setBigNumberLength(&tmpResult2);

		while((i+ELEMENTS_BIG_HEX) < (MAX_OCTETS/2))
		{
			specialModP(&tmpResult2);
			shiftLeftOneArrayElement(&tmpResult2);

			// Add the next byte from A in left_over;
			tmpResult2.num[HIGHEST_INDEX_BIG_HEX] = tmpResult.num[i+ELEMENTS_BIG_HEX];
			i++;
			setBigNumberLength(&tmpResult2);
		}
		specialModP(&tmpResult2);
		copyBigHex(&tmpResult2,BigHexResult);
	}
}


void MultiplyBigHexByUint32(const bigHex *bigHexA, const uint32_t numB, bigHex *BigHexResult)
{
	int32_t k;
	const uint32_t *numA = bigHexA->num;
	uint32_t *result = BigHexResult->num;
	uint32_t mcarry = 0;

	//
	// Below Optimisations seem to degrade performance on Windows by a factor of 2.
	//
	for(k=HIGHEST_INDEX_BIG_HEX;k >= 0;k--)
	{
		u64 val;

		val = (((u64)(numA[k]) ) * ((u64)numB)) + mcarry;
		result[k] = (val & 0xFFFFFFFF);
		mcarry = (uint32_t)( val >> 32);
	}
	setBigNumberLength(BigHexResult);
	BigHexResult->sign = bigHexA->sign;
	specialModP(BigHexResult);
}


void shiftLeftOneArrayElement(bigHex *input)
{
	memmove(input->num,(input->num+1), (6 * sizeof(uint32_t)));
}

//
// This function adds N * Mod P to a number.
//
// The left_over = tmpHexA % P

void specialModP(bigHex *tmpHexA)
{

   if(((tmpHexA->sign == 0) && isGreaterThanOrEqual(tmpHexA,&bigHexP)) ||
	   (tmpHexA->sign == 1))
   {
	   // If I multiply the most significant u_int16 of A by the Mod P and then subtract from A
	   // this is equivalent to an iteractive subtraction - but much faster.
	   if (tmpHexA->len > bigHexP.len)
	   {
		   bigHex NModPinv;
		   uint32_t SignificantU32 = GETMOSTSIGNIGICANTU32(tmpHexA);

		   initBigNumber(&NModPinv);

           MultiplyByU32ModPInv(SignificantU32,&NModPinv);
		   tmpHexA->num[0] = 0x00;

		   if (tmpHexA->sign == 0)
		   {
				Add2SelfBigHex(tmpHexA,&NModPinv);
		   }
		   else
		   {
				SubtractFromSelfBigHexSign(tmpHexA,&NModPinv);
		   }
	   }
	   if (((tmpHexA->sign == 0) && isGreaterThanOrEqual(tmpHexA,&bigHexP)) ||
			(tmpHexA->sign == 1))
	   {
		 // It is implicit that the sign is negative.
	     // so we can remove the check.
		 //
		   if (tmpHexA->sign == 1)
		   {
			   AddP(tmpHexA);
		   }
		   else
		   {
			   // Can this be replaced by Adding the invert of P
			   SubtractFromSelfBigHex(tmpHexA, &bigHexP);
		   }
	   }
   }
}

void MultiplyByU32ModPInv(const uint32_t N,bigHex* result)
{
	u64 tempVal;

	tempVal = (u64)((N << 1) & _LL(0x1FFFFFFFE));

    result->num[6] = N;
	result->num[4] = N;
	result->num[3] = (uint32_t) ((tempVal>>32) & 0xF);

	result->sign = 0x00;
	if (result->num[3])
		result->len = 0x4;
	else
		result->len = 0x3;
}

void copyBigHex(const bigHex *source,bigHex *destination)
{
	memcpy(destination->num,source->num, sizeof(destination->num) );

	destination->len = source->len;
	destination->sign = source->sign;
}

void initBigNumber(bigHex *BigHexResult)
{
	memset(BigHexResult->num,0, sizeof(BigHexResult->num) );

	BigHexResult->len = 0;
	BigHexResult->sign = 0;
}

void setBigNumberLength(bigHex *BigHexResult)
{
	int i;

	for(i=0;i<(ELEMENTS_BIG_HEX);i++)
	{
		if(BigHexResult->num[i] != 0x00)
           break;
	}
	BigHexResult->len = (ELEMENTS_BIG_HEX)-i;
}

void setVeryBigNumberLength(veryBigHex *BigHexResult)
{
	int i;

	for(i=0;i<(MAX_OCTETS/2);i++)
	{
		if(BigHexResult->num[i] != 0x00)
           break;
	}
	BigHexResult->len = (MAX_OCTETS/2)-i;
}

//
// if A > B return TRUE
//

int isGreaterThan(const bigHex *bigHexA,const bigHex *bigHexB)
{

   uint32_t i;
   uint32_t A_len = bigHexA->len;
   uint32_t B_len = bigHexB->len;

    if(A_len > B_len)
		return 1;
	else if(A_len < B_len)
		return 0;

	// Now we have two equal sized arrays which we need to compare.
    // .
	for (i=((ELEMENTS_BIG_HEX)-A_len); i <(ELEMENTS_BIG_HEX);i++)
	{
		if (bigHexB->num[i] > bigHexA->num[i])
		{
			return 0;
		}
		else if (bigHexB->num[i] < bigHexA->num[i])
		{
			return 1;
		}
	}
	return 0;
}


int isGreaterThanOrEqual(const bigHex *bigHexA,const bigHex *bigHexB)
{
   uint32_t i;
   uint32_t A_len = bigHexA->len;
   uint32_t B_len = bigHexB->len;

    if(A_len > B_len)
		return 1;
	else if(A_len < B_len)
		return 0;

	// Now we have two equal sized arrays which we need to compare.
    // .

	for (i=((ELEMENTS_BIG_HEX)-A_len); i <(ELEMENTS_BIG_HEX);i++)
	{
		if (bigHexB->num[i] > bigHexA->num[i])
		{
			return 0;
		}
		else if (bigHexB->num[i] < bigHexA->num[i])
		{
			return 1;
		}
	}
	return 1;
}


int isVeryBigHexGreaterThanOrEqual(const veryBigHex *bigHexA,const veryBigHex *bigHexB)
{
	int i;
	uint32_t A_len = bigHexA->len;
	uint32_t B_len = bigHexB->len;

    if(A_len > B_len)
		return 1;
	else if(A_len < B_len)
		return 0;

	// Now we have two equal sized arrays which we need to compare.
    //
	for (i=((MAX_OCTETS/2)-A_len); i <(MAX_OCTETS/2);i++)
	{
		if (bigHexB->num[i] > bigHexA->num[i])
		{
			return 0;
		}
		else if (bigHexB->num[i] < bigHexA->num[i])
		{
			return 1;
		}
	}
	return 1;
}
int notEqual(const bigHex *bigHexA,const bigHex *bigHexB)
{
	uint32_t i;

	for(i=0;i<(ELEMENTS_BIG_HEX);i++)
	{
		if(bigHexA->num[i] != bigHexB->num[i])
		{
			return 1;
		}
	}
	return 0;
}

int Is_Infinite(const ECC_Jacobian_Point* pPointP)
{
	return ( (!notEqual(&pPointP->x,&LMecc_Jacobian_InfinityPoint.x)) && (!notEqual(&pPointP->y,&LMecc_Jacobian_InfinityPoint.y)) && (!notEqual(&pPointP->z,&LMecc_Jacobian_InfinityPoint.z)));
}


//isLessThan
int isLessThan(const bigHex *bigHexA,const bigHex *bigHexB)
{
	uint32_t i;

	if(bigHexA->len < bigHexB->len)
		return 1;
	else if(bigHexA->len > bigHexB->len)
		return 0;

	// Now we have two equal sized arrays which we need to compare.
    // Start at the last entry is each array and compare.

	for (i=bigHexA->len; i!=0;i--)
	{
		if (bigHexB->num[i] < bigHexA->num[i])
			return 0;
	}
	return 1;
}

#ifdef JACOBIAN_METHOD
/*********** JACOBIAN ******/
int GF_Jacobian_Point_Addition(const ECC_Jacobian_Point* pPointP,const ECC_Jacobian_Point* pPointQ, ECC_Jacobian_Point* pResultPoint)
{

// This should be 12 Multiplications and 2 Squares
// currently implemented as 12 Muls and 2 Squares and one cube
//  Let (X1, Y1, Z1) and (X2, Y2, Z2) be two points (both unequal to the 'point at infinity') represented in 'Standard Projective Coordinates'. Then the sum (X3, Y3, Z3) can be calculated by
//
//  U1 = Y2*Z1
//  U2 = Y1*Z2
//  V1 = X2*Z1
//  V2 = X1*Z2
//  if (V1 == V2)
//     if (U1 != U2)
//         return POINT_AT_INFINITY
//      else
//         return POINT_DOUBLE(X1, Y1, Z1)
// U = U1 - U2
// V = V1 - V2
// W = Z1*Z2

// A = U^2*W - V^3 - 2*V^2*V2
// X3 = V*A
// Y3 = U*(V^2*V2 - A) - V^3*U2
// Z3 = V^3*W
//  return (X3, Y3, Z3)

	bigHex jac_U1;
	bigHex jac_U2;
	bigHex jac_V1;
	bigHex jac_V2;
	bigHex jac_U;
	bigHex jac_V;
	bigHex jac_A;
	bigHex jac_W;
	bigHex V_sqr;
	bigHex V_cube;
	bigHex U_sqr;
 	bigHex V_sqr_mult_by_V2;

	const bigHex* pJac_PointX1 = &pPointP->x;
	const bigHex* pJac_PointY1 = &pPointP->y;
	const bigHex* pJac_PointZ1 = &pPointP->z;

	const bigHex* pJac_PointX2 = &pPointQ->x;
	const bigHex* pJac_PointY2 = &pPointQ->y;
	const bigHex* pJac_PointZ2 = &pPointQ->z;

	// First Handle points of infinity
/*
P1 and P2 are both infinite: P3=infinite.
P1 is infinite: P3=P2.
P2 is infinite: P3=P1.
P1 and P2 have the same x coordinate, but different y coordinates or both y coordinates equal 0: P3=infinite.

*/
	if (Is_Infinite(pPointP))
	{
		if(Is_Infinite(pPointQ))
		{
			// Result = Infinity
			//pResultPoint = Infinity
			GF_Jacobian_Point_Copy(&LMecc_Jacobian_InfinityPoint,pResultPoint);
			return 0;
		}
		else
		{
			// Result = pPointQ
			// pResultPoint = pPointQ
			GF_Jacobian_Point_Copy(pPointQ,pResultPoint);
			return 0;
		}
	}
	else if (Is_Infinite(pPointQ))
	{
		// Result = pPointP
		// pResultPoint = pPointP;
		GF_Jacobian_Point_Copy(pPointP,pResultPoint);
		return 0;
	}

	if (!notEqual(&pPointQ->x,&pPointP->x)) // Same X coordinate
	{
		if (notEqual(&pPointQ->y,&pPointP->y)) // Different Y coordinates
		{
			// Result = Infinity
			GF_Jacobian_Point_Copy(&LMecc_Jacobian_InfinityPoint,pResultPoint);
			return 0;

		}
		else if ((pPointQ->y.len <= 1) && (pPointQ->y.num[6] == 0)) // Y co-ords = 0
		{
			// Result = Infinity
			GF_Jacobian_Point_Copy(&LMecc_Jacobian_InfinityPoint,pResultPoint);
			return 0;
		}
	}

//  U1 = Y2*Z1
//  U2 = Y1*Z2
//  V1 = X2*Z1
//  V2 = X1*Z2

	MultiplyBigHexModP(pJac_PointY2,pJac_PointZ1,&jac_U1);
	MultiplyBigHexModP(pJac_PointY1,pJac_PointZ2,&jac_U2);
	MultiplyBigHexModP(pJac_PointX2,pJac_PointZ1,&jac_V1);
	MultiplyBigHexModP(pJac_PointX1,pJac_PointZ2,&jac_V2);

//  if (V1 == V2)
//     if (U1 != U2)
//         return POINT_AT_INFINITY
//      else
//         return POINT_DOUBLE(X1, Y1, Z1)

	if (!notEqual(&jac_V1,&jac_V2))
	{
		if (notEqual(&jac_U1,&jac_U2))
		{
			// Result = Infinity
			GF_Jacobian_Point_Copy(&LMecc_Jacobian_InfinityPoint,pResultPoint);
			return 0;
		}
		else
		{
		    GF_Jacobian_Point_Double(pPointP,pResultPoint);
		    return 0; //POINT_DOUBLE(&jac_PointX1,&jac_PointY1,&jac_PointZ1);
		}
	}

// U = U1 - U2
// V = V1 - V2
// W = Z1*Z2   ( 5 muls to here )
// A = U^2*W - V^3 - 2*V^2*V2

	SubtractBigHexMod(&(jac_U1),&(jac_U2),&jac_U);
	SubtractBigHexMod(&(jac_V1),&(jac_V2),&jac_V);

	MultiplyBigHexModP(pJac_PointZ1,pJac_PointZ2,&jac_W);

	{
		// Determine A
		// A = U^2*W - V^3 - 2*V^2*V2
		// V_sqr = V^2
		// V_cube = V^3
		// U_sqr = U^2
		// int1 = V_sqr * V2
		// int2 = 2 * int1  --  (2*V^2*V2)
		// int3 = U_sqr * W
		// int4 = int3 - V_cube
		// A = int4 - int2

		// X3 = D^2 - int3

//		bigHex int1;
		bigHex int3;
		bigHex int4;
		bigHex Double_V_sqr_mult_by_V2;

		MultiplyBigHexModP(&jac_V,&jac_V,&V_sqr);
		MultiplyBigHexModP(&V_sqr,&jac_V,&V_cube);
		MultiplyBigHexModP(&jac_U,&jac_U,&U_sqr);

		MultiplyBigHexModP(&V_sqr,&jac_V2,&V_sqr_mult_by_V2);  // V^2 * V2

        MultiplyBigHexByUint32(&V_sqr_mult_by_V2,0x02,&Double_V_sqr_mult_by_V2); // 2 * V^2 * V2
		MultiplyBigHexModP(&U_sqr,&jac_W,&int3);    // U^2 * W
		SubtractBigHexMod(&int3,&V_cube,&int4);   // (U^2 * W) - V^3
		SubtractBigHexMod(&int4,&Double_V_sqr_mult_by_V2,&jac_A);  // A = U^2*W - V^3 - 2*V^2*V2
	}

	// Determine X3
	{
		// X3 = V*A

		MultiplyBigHexModP(&jac_V,&jac_A,&(pResultPoint->x));
	}
	// Determine Y3
	{
	   // Y3 = U*(V^2*V2 - A) - V^3*U2
	   // int1 = (V^2*V2 - A)
	   // int2 = U*(V^2*V2 - A)
	   // int3 =  V^3*U2
		bigHex int1;
		bigHex int2;
		bigHex int3;

	    SubtractBigHexMod(&V_sqr_mult_by_V2,&jac_A,&int1); // int1 =( V^2*V2 - A)
		// Below line was incorrect
	    //   MultiplyBigHexModP(&jac_U,&V_sqr_mult_by_V2,&int2);  // Error here
		 MultiplyBigHexModP(&jac_U,&int1,&int2);  // int2 = U * ( V^2*V2 - A)
	    //
		MultiplyBigHexModP(&V_cube,&jac_U2,&int3); // int3 = V^3 * U2
		SubtractBigHexMod(&int2,&int3,&(pResultPoint->y));
	}


	// Determine Z3
	{
		// Z3 = V^3*W
		MultiplyBigHexModP(&V_cube,&jac_W,&(pResultPoint->z));
	}

	return 1;
}

int GF_Jacobian_Point_Double(const ECC_Jacobian_Point* pPointP, ECC_Jacobian_Point* pResultPoint)
{
 //if (Y == 0)
 // return POINT_AT_INFINITY
 //W = a*Z^2 + 3*X^2
 // if a = -3, then W can also be calculated as W = 3*(X + Z)*(X - Z), saving 2 field squarings
 // THUS
 //W = 3*(X+Z)*(X-Z)
 //S = Y*Z
 //B = X*Y*S
 //H = W^2 - 8*B
 //X' = 2*H*S
 //Y' = W*(4*B - H) - 8*Y^2*S^2
 //Z' = 8*S^3
 //return (X', Y', Z')

	const bigHex* pJac_PointX1 = &pPointP->x;
	const bigHex* pJac_PointY1 = &pPointP->y;
	const bigHex* pJac_PointZ1 = &pPointP->z;

	bigHex W;
	bigHex S;
	bigHex B;
	bigHex H;
	bigHex S_sqr;

	// Determine W
	{
		bigHex X_plus_Z;
		bigHex X_minus_Z;
		bigHex int1;

		AddBigHexModP(pJac_PointX1,pJac_PointZ1,&X_plus_Z);
		SubtractBigHexMod(pJac_PointX1,pJac_PointZ1,&X_minus_Z);
		MultiplyBigHexModP(&X_plus_Z,&X_minus_Z,&int1);
		MultiplyBigHexByUint32(&(int1),0x03,&W);
	}
	// Determine S
	{
		MultiplyBigHexModP(pJac_PointY1,pJac_PointZ1,&S);
	}
	// Determine B
	{
		bigHex int1;

		//B = X*Y*S
		MultiplyBigHexModP(pJac_PointY1,&S,&int1);
		MultiplyBigHexModP(pJac_PointX1,&int1,&B);
	}

	// Determine H
	{
	    //H = W^2 - 8*B
	 	bigHex W_sqr;
	 	bigHex Eight_B;

	 	MultiplyBigHexModP(&W,&W,&W_sqr);
        MultiplyBigHexByUint32(&B,8,&Eight_B);

		SubtractBigHexMod(&W_sqr,&Eight_B,&H);
	}
	// Determine X
    //X = 2*H*S
    {
    	bigHex int1;

    	MultiplyBigHexModP(&H,&S,&int1);
    	MultiplyBigHexByUint32((const bigHex*)(&int1),(const uint32_t)0x02,&pResultPoint->x);

    }
    // Determine Y
    // Y' = W*(4*B - H) - 8*Y^2*S^2

	{
		bigHex Y_sqr;
		bigHex int1;
		bigHex int2; // 8*Y^2*S^2
		bigHex int3;
		bigHex int4;
		bigHex int5;

		// 8*Y^2*S^2
		MultiplyBigHexModP(pJac_PointY1,pJac_PointY1,&Y_sqr);
		MultiplyBigHexModP(&S,&S,&S_sqr);
		MultiplyBigHexModP(&Y_sqr,&S_sqr,&int1);
		MultiplyBigHexByUint32(&(int1),0x08,&int2);

		// 4 * B
		MultiplyBigHexByUint32(&B,0x04,&int3);
		// (4*B - H)
		SubtractBigHexMod(&int3,&H,&int4);
		// W * (4*B - H)
		MultiplyBigHexModP(&W,&int4,&int5);

		SubtractBigHexMod((const bigHex*)&int5,(const bigHex*)&int2,&pResultPoint->y);

	}

	// Determine Z
	{
	  	//Z1 = 8*S^3
	  	bigHex int1;

	  	MultiplyBigHexModP(&S_sqr,&S,&int1);
	  	MultiplyBigHexByUint32(&int1,0x08,&pResultPoint->z);


	}

	return 0;
}
/********************************************************
 * Function :- GF_Point_Jacobian_To_Affine
 *
 * Parameters :- pJacPoint - a pointer to a Jacobian Point which is to be mapped to Affine.
 *               pX_co_ord_affine  - pointer to the resultant Affine X-coordinate
  *              pY_co_ord_affine  - pointer to the resultant Affine Y-coordinate
 *
 * Description
 * This function maps a Jacobian point (x,y,z) back to an affine (x,y).
 *   x_affine = X_jacobian/Z_jacobian
 *   y_affine = Y_jacobian/Z_jacobian
 *
 * The computational intensive part is determining 1/Z
 ********************************************************/

void GF_Point_Jacobian_To_Affine(ECC_Jacobian_Point* pJacPoint,bigHex* pX_co_ord_affine,bigHex* pY_co_ord_affine )
{
	bigHex inverted_Z_jacobian;

	// First Determine 1/Z_jacobian
	bigHexInversion(&pJacPoint->z,&inverted_Z_jacobian);

	MultiplyBigHexModP(&pJacPoint->x,&inverted_Z_jacobian,pX_co_ord_affine);
	MultiplyBigHexModP(&pJacPoint->y,&inverted_Z_jacobian,pY_co_ord_affine);

}
#endif
//    Q = kP
//
/********************************************************
 * Function :- ECC_Point_Multiplication
 *
 * Parameters :- pk       - a pointer to a bigHex which the point should be multiplied by.
 *               pPointP  - pointer to the point which is to be multiplied by pk
 *
 * Description
 * This function performs ECC point multiplication. It uses the Scalar Multiplication
 * algorithm. Scalar multiplication works is way through the bits of a BigHex starting with the LSB.
 * For each bit (irrespective of Value) a point Doubling is performed, if the bit has a value = 1 then
 * a point addition is also performed.
 *
 * Scalar Multiplication: LSB first
 *   • Require k=(km-1,km-2,…,k0)2, km=1
 *   • Compute Q=kP
 * – Q=0, R=P
 * – For i=0 to m-1
 *   • If ki=1 then
 *      – Q=Q+R
 *   • End if
 *   • R=2R
 * – End for
 * – Return Q
 ******************************************************************************************/
void ECC_Point_Multiplication(const bigHex* pk,const ECC_Point* pPointP, void* p_link,uint8_t blocking)
{
	// see ecc.pdf
	//
	ECC_Jacobian_Point PointP_Jacobian;
	ECC_Jacobian_Point* pPointP_Jacobian = &PointP_Jacobian;
	// Need to map from Affine Point to Jacobian Point
 	GF_Affine_To_Jacobian_Point_Copy(pPointP,pPointP_Jacobian);

	GF_Jacobian_Point_Copy(pPointP_Jacobian,&LMecc_Jacobian_PointP);
	GF_Jacobian_Point_Copy(pPointP_Jacobian,&LMecc_Jacobian_PointR);

	copyBigHex(pk,&LMecc_Pk);

	initBigNumber(&LMecc_PointQ.x);
	initBigNumber(&LMecc_PointQ.y);

	GF_Setup_Jacobian_Infinity_Point(&LMecc_Jacobian_PointQ);

	ECC_Point_Mul_Word = ELEMENTS_BIG_HEX*4;
   // ECC_Point_Multiplication_uint32(p_link)


	if (blocking)
	{
		int i;

		for(i = ECC_Point_Mul_Word; i > 0; i --)
			ECC_Point_Multiplication_uint8(p_link,blocking);
	}
	else
		ECC_Point_Multiplication_uint8(p_link,blocking);

}

void LMecc_CB_ECC_Point_Multiplication_Complete(void* p_link)
{
	uint32_t big_num_offset;
	uint32_t i,j=0;

        (void*)p_link;

	// Copy the secret_key to a bigHex format.
	big_num_offset = 1;

	GF_Point_Jacobian_To_Affine(&LMecc_Jacobian_PointQ,&LMecc_PointQ.x,&LMecc_PointQ.y);

	/*if (p_link)
	{
		for(i=0;i<24;)
		{
			p_link->DHkey[i] =   ((LMecc_PointQ.x.num[j+big_num_offset] & 0xFF000000) >> 24);
			p_link->DHkey[i+1] = ((LMecc_PointQ.x.num[j+big_num_offset] & 0xFF0000) >> 16);
			p_link->DHkey[i+2] = ((LMecc_PointQ.x.num[j+big_num_offset] & 0xFF00) >> 8);
			p_link->DHkey[i+3] = ( LMecc_PointQ.x.num[j+big_num_offset] & 0xFF);

			i+=4;
			j++;
		}

		// Need to invoke logic in the SSP protocol in case this calculation complete comes after the
		// User_Confirmation_Request_Reply.

		LMssp_CallBack_DH_Key_Complete(p_link);
	}
	else // p_link == 0  then local device.*/
	{
		for(i=0;i<24;)
		{
			public_key_x[i] =   ((LMecc_PointQ.x.num[j+big_num_offset] & 0xFF000000) >> 24);
			public_key_x[i+1] = ((LMecc_PointQ.x.num[j+big_num_offset] & 0xFF0000) >> 16);
			public_key_x[i+2] = ((LMecc_PointQ.x.num[j+big_num_offset] & 0xFF00) >> 8);
			public_key_x[i+3] = ( LMecc_PointQ.x.num[j+big_num_offset] & 0xFF);

			public_key_y[i] =   ((LMecc_PointQ.y.num[j+big_num_offset] & 0xFF000000) >> 24);
			public_key_y[i+1] = ((LMecc_PointQ.y.num[j+big_num_offset] & 0xFF0000) >> 16);
			public_key_y[i+2] = ((LMecc_PointQ.y.num[j+big_num_offset] & 0xFF00) >> 8);
			public_key_y[i+3] = ( LMecc_PointQ.y.num[j+big_num_offset] & 0xFF);

			i+=4;
			j++;
		}
	}
}


void ECC_Point_Multiplication_uint32(void* p_link)
{

	ECC_Jacobian_Point tmpResultPoint;
	ECC_Jacobian_Point tmpResultPoint2;

	uint32_t j;
	uint32_t bitVal;

	for (j=0;j<32;j++)
	{
		bitVal = (LMecc_Pk.num[ECC_Point_Mul_Word-1] >> j) & 0x0001;

		if (bitVal)
		{
			// Q = Q + R
            GF_Jacobian_Point_Addition(&LMecc_Jacobian_PointQ,&LMecc_Jacobian_PointR,&tmpResultPoint);
			// Copy ResultPoint to PointQ
			GF_Jacobian_Point_Copy(&tmpResultPoint,&LMecc_Jacobian_PointQ);
		}
		// Point Doubling
		// R = 2R
		GF_Jacobian_Point_Double(&LMecc_Jacobian_PointR,&tmpResultPoint2);
		// Copy Result point to PointR
		GF_Jacobian_Point_Copy(&tmpResultPoint2,&LMecc_Jacobian_PointR);
	}

	// We Set a Timer - for the next Multiplication of next uint32_t

	if (ECC_Point_Mul_Word == 1)
	{
		// Call Back a function which will store the ECC calculation result and
		// flag that the calculation is complete.

		LMecc_CB_ECC_Point_Multiplication_Complete(p_link);
	}
	else
	{
	    ECC_Point_Mul_Word--;
		///LMtmr_Set_Timer(4,ECC_Point_Multiplication_uint32, p_link, 1);
	}

}

void ECC_Point_Multiplication_uint8(void* p_link,uint8_t blocking)
{

	ECC_Jacobian_Point tmpResultPoint;
	ECC_Jacobian_Point tmpResultPoint2;

	uint32_t j;
	uint32_t bitVal;
	uint8_t byte=0;
	uint8_t byteOffset;

	uint32_t wordVal;

        (void)blocking;

	wordVal = LMecc_Pk.num[((ECC_Point_Mul_Word+3)/4)-1];
	byteOffset = (ECC_Point_Mul_Word-1)%4;

	switch(byteOffset)
	{
	case 3:
		byte = wordVal & 0xFF;
		break;
	case 2:
		byte = (wordVal & 0xFF00)>>8;
		break;
	case 1:
		byte = (wordVal & 0xFF0000)>>16;
		break;
	case 0:
		byte = (wordVal & 0xFF000000)>>24;
		break;
	}

	for (j=0;j<8;j++)
	{
		bitVal = (byte >> j) & 0x0001;

		if (bitVal)
		{
			// Q = Q + R
			GF_Jacobian_Point_Addition(&LMecc_Jacobian_PointQ,&LMecc_Jacobian_PointR,&tmpResultPoint);
			// Copy ResultPoint to PointQ
			GF_Jacobian_Point_Copy(&tmpResultPoint,&LMecc_Jacobian_PointQ);
		}
		// Point Doubling
		// R = 2R
		GF_Jacobian_Point_Double(&LMecc_Jacobian_PointR,&tmpResultPoint2);
		// Copy Result point to PointR
		GF_Jacobian_Point_Copy(&tmpResultPoint2,&LMecc_Jacobian_PointR);

	}

	// We Set a Timer - for the next Multiplication of next uint32_t

	if (ECC_Point_Mul_Word == 1)
	{
		// Call Back a function which will store the ECC calculation result and
		// flag that the calculation is complete.

		LMecc_CB_ECC_Point_Multiplication_Complete(p_link);
	}
	else
	{
	    ECC_Point_Mul_Word--;

		///if (!blocking)
			///LMtmr_Set_Timer(4,ECC_Point_Multiplication_uint8_non_blocking, p_link, 1);
	}

}


void ECC_Point_Multiplication_uint8_non_blocking(void* p_link )
{
	ECC_Point_Multiplication_uint8(p_link,FALSE);
}

void GF_Point_Copy(const ECC_Point *source,ECC_Point *destination)
{
	copyBigHex(&source->x,&destination->x);
	copyBigHex(&source->y,&destination->y);
}
#ifdef JACOBIAN_METHOD


void GF_Jacobian_Point_Copy(const ECC_Jacobian_Point *source,ECC_Jacobian_Point *destination)
{
	copyBigHex(&source->x,&destination->x);
	copyBigHex(&source->y,&destination->y);
	copyBigHex(&source->z,&destination->z);
}

void GF_Affine_To_Jacobian_Point_Copy(const ECC_Point *source,ECC_Jacobian_Point *destination)
{
	bigHex BigHex_1;

	BigHex_1.len = 0x01;
	BigHex_1.sign = 0; // Positive Number
	memset(BigHex_1.num,0, sizeof(BigHex_1.num) );
	BigHex_1.num[6] = 0x01;
	copyBigHex(&source->x,&destination->x);
	copyBigHex(&source->y,&destination->y);
	copyBigHex(&BigHex_1,&destination->z);
}

void GF_Setup_Jacobian_Infinity_Point(ECC_Jacobian_Point *infinity)
{
	bigHex BigHex_1;

	BigHex_1.len = 0x01;
	BigHex_1.sign = 0; // Positive Number
	memset(BigHex_1.num,0, sizeof(BigHex_1.num) );
	BigHex_1.num[6] = 0x00;
	copyBigHex(&BigHex_1,&infinity->x);
	BigHex_1.num[6] = 0x01;
	copyBigHex(&BigHex_1,&infinity->y);
	BigHex_1.num[6] = 0x00;
	copyBigHex(&BigHex_1,&infinity->z);

}

#endif
/**************************************************************
 *  Function :- bigHexInversion
 *
 *  Parameters :-  bigHexA - a pointer to bigHex - the input
 *                 pResult - a pointer to bigHex - the output
 *
 *  Description
 *  This function performs the inversion of a bigHex number. The output multiplied
 *  by the input will result in '1', as the output is Moded by P.
 *
 *            ( bigHexA * pResult ) % P = 1.
 *
 * Ref " Software Implementation of the NIST Elliptical "
 *  Input : Prime P (bigHexP), a (bigHexA) E [1, p-1]
 *  Output : a^-1 mod P
 ************************************************************************/

void bigHexInversion( bigHex* bigHexA,bigHex* pResult)
{
	bigHex u;
	bigHex v;
	bigHex A;
	bigHex C;

	// Change the sign to positive
	bigHexA->sign = 0;

	// Step 1
	// u <-- a, v <-- p, A <-- 1, C <-- 0.
	//
	initBigNumber(&A);
	initBigNumber(&C);

	copyBigHex(bigHexA,&u);
	copyBigHex(&bigHexP,&v);

	A.num[HIGHEST_INDEX_BIG_HEX] = 1;
	C.num[HIGHEST_INDEX_BIG_HEX] = 0;
	A.len = 1;
	C.len = 1;

    //
	// Step 2.
    //   While u != 0 do
	//   2.1  While u is even do :
	//           u <-- u/2. If A is even then A <-- A/2; else A <-- (A+P)/2
	//   2.2  While v is even do :
	//           v <-- v/2. If C is even then C <-- C/2; else C <-- (C+P)/2
	//   2.3  If u >= v then : u <-- u - v, A <-- A - C; else v <-- v - u, C <-- C - A
	//
	while(u.len!=0)
	{
		while(ISBIGHEXEVEN(u))
		{
			divideByTwo(&u);
			if (ISBIGHEXEVEN(A))
			{
				// A = A/2
                divideByTwo(&A);
			}
			else
			{
                AddPdiv2(&A);
			}
		}
		while(ISBIGHEXEVEN(v))
		{
			divideByTwo(&v);
			if (ISBIGHEXEVEN(C))
			{
                divideByTwo(&C);
			}
			else
			{
                AddPdiv2(&C);
			}
		}
		if (isGreaterThanOrEqual(&u,&v))
		{
			SubtractFromSelfBigHex(&u,&v);
			SubtractFromSelfBigHexSign(&A,&C);
		}
		else
		{
			SubtractFromSelfBigHex(&v,&u);
			SubtractFromSelfBigHexSign(&C,&A);
		}
	}
	// Step 3 :- Now perform Mod P
	// pResult = C % P
	{
		// If the Mod P is greater than bigHexA then return with
		// C unmodified.

		if (C.sign == 0)
		{
			if (isGreaterThan(&bigHexP,&C))
			{
				copyBigHex(&C,pResult);
				return;
			}
			else // Positive Number thus subtract P iteratively.
			{
				specialModP(&C);
			}
		}
		else // Negative Number
		{
			specialModP(&C);
		}
		copyBigHex(&C,pResult);
	}
}

/*******************************************************************
 * Funcion :- divideByTwo
 *
 * Parameters :- A - a bigHex pointer - which is the divided by two
 *
 *******************************************************************/

void divideByTwo(bigHex* A)
{
    uint32_t rem=0;
	u64 u64val;
    uint32_t i;

	for(i=0; i < (ELEMENTS_BIG_HEX); i++)
	{
		u64val = (u64)((u64)(A->num[i]) + ((u64)rem * _LL(0x100000000)));

		A->num[i] = (uint32_t)(u64val>>1);
        rem = (u64)(u64val - ((u64val>>1)<<1));
	}
	// We need to re-calculate the length.
    setBigNumberLength(A);
}

void LMecc_Generate_ECC_Key(const uint8_t* secret_key, const uint8_t* public_key_x,
					   const uint8_t* public_key_y,void* p_link,uint8_t blocking)
{
	uint32_t big_num_offset=1;
	uint32_t i;

	// Now Copy the Public Key and Secret Key coordinates to ECC point format.
	PrivateKey.num[0] = 0;
	PublicKey.x.num[0] = 0;
	PublicKey.y.num[0] = 0;

	for (i=0;i<24;)
	{
		PrivateKey.num[(i/4)+big_num_offset] = (uint32_t)
		                                            (((*(secret_key+i    )) << 24) & 0xFF000000) +
			                                        (((*(secret_key+i+1  )) << 16) & 0x00FF0000) +
			                                        (((*(secret_key+i+2  )) <<  8) & 0x0000FF00) +
													(((*(secret_key+i+3  ))        & 0x000000FF));

		PublicKey.x.num[(i/4)+big_num_offset] = (uint32_t)
		                                          ( (((*(public_key_x+i    )) << 24) & 0xFF000000) +
			                                        (((*(public_key_x+i+1  )) << 16) & 0x00FF0000) +
			                                        (((*(public_key_x+i+2  )) <<  8) & 0x0000FF00) +
													(((*(public_key_x+i+3  )))        & 0x000000FF));

		PublicKey.y.num[(i/4)+big_num_offset] = (uint32_t)
		                                          ( (((*(public_key_y+i    )) << 24) & 0xFF000000) +
			                                        (((*(public_key_y+i+1  )) << 16) & 0x00FF0000) +
			                                        (((*(public_key_y+i+2  )) <<  8) & 0x0000FF00) +
													(((*(public_key_y+i+3  )))        & 0x000000FF));
		i += 4;
	}
	setBigNumberLength(&PrivateKey);
	setBigNumberLength(&PublicKey.x);
	setBigNumberLength(&PublicKey.y);
	PublicKey.x.sign = 0;
	PublicKey.y.sign = 0;

	ECC_Point_Multiplication(&PrivateKey,&PublicKey,p_link,blocking);
}


void LMecc_Get_Public_Key(uint8_t* key_x, uint8_t* key_y)
{
	memcpy(key_x, public_key_x, sizeof(public_key_x));
	if (key_y != NULL)
		memcpy(key_y, public_key_y, sizeof(public_key_y));
}

// For ARM build below lines return the system to default build optimisation
//#pragma Ospace
//#pragma O0

///#else

///#endif
