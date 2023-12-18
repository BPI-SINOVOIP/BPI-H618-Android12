/******************************************************************************
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 ******************************************************************************/

#ifndef _SDD_IF_H_
#define _SDD_IF_H_

#include "sdd_defs.h"

/*======================================================================*/
/* Access Routines for SDD                                              */
/*======================================================================*/

typedef void SDD_SECT_HANDLER(SDD_GENERAL_HEADER_ELT *pIE, uint32_t length);

typedef struct SDD_SECT_ID_AND_HANDLE {
    uint8_t               Id;
    SDD_SECT_HANDLER    *Handler;
} SDD_SECT_ID_AND_HANDLE;

/*======================================================================*/
/* Access Routines for SDD                                              */
/*======================================================================*/

/* Get next IE with Length bytes remaining */
/* NULL if none left */
SDD_GENERAL_HEADER_ELT *SDD_GetNextIE(SDD_GENERAL_HEADER_ELT *pIE, uint32_t *pLength);

uint8_t *SDD_WriteSection(uint8_t *pIE, int sectionId, uint8_t *(pWriteSectIes)(uint8_t *pIE));

uint8_t *SDD_WriteSectIe(uint8_t *pIE, int type, int len, uint8_t *pData);

uint8_t *SDD_RegenerateLastSection(uint8_t *pIE);

uint8_t *SDD_FindSection(uint8_t *pSDD, uint8_t id);

#endif // _SDD_IF_H_


/*************************************************************************\
                                EOF
\*************************************************************************/

