#ifndef ECC_HEADER
#define ECC_HEADER
/******************************************************************************
 * MODULE NAME:    lmp_ecc.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:
 *
 * SOURCE CONTROL: $ $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2009 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/
#include <stdint.h>

#define MAX_DIGITS 56
#define MAX_OCTETS 28


 extern const uint8_t BasePoint_x[24];

 extern const uint8_t BasePoint_y[24];

#define FALSE 0
#define TRUE  1

typedef struct bigHex
{
	uint32_t num[MAX_OCTETS/4];
	uint32_t  len;
	uint32_t  sign;
} bigHex;

typedef struct veryBigHex
{
	uint32_t num[MAX_OCTETS/2];
	uint32_t  len;
	uint32_t  sign;
} veryBigHex;

typedef struct ECC_Point
{
	bigHex x;
	bigHex y;
} ECC_Point;

typedef struct ECC_Jacobian_Point
{
	bigHex x;
	bigHex y;
	bigHex z;
} ECC_Jacobian_Point;


void ECC_Point_Multiplication(const bigHex* pk,const ECC_Point* pPointP, void* p_link,uint8_t blocking);
int notEqual(const bigHex *bigHexA,const bigHex *bigHexB);
void GF_Point_Copy(const ECC_Point *source,ECC_Point *destination);
void GF_Jacobian_Point_Copy(const ECC_Jacobian_Point *source,ECC_Jacobian_Point *destination);
void LMecc_Generate_ECC_Key(const uint8_t* secret_key, const uint8_t* public_key_x, const uint8_t* public_key_y, void* p_link,uint8_t blocking);
int LMecc_isValidSecretKey(uint8_t* secretKey);
void LMecc_Get_Public_Key(uint8_t* key_x, uint8_t* key_y);

#endif
