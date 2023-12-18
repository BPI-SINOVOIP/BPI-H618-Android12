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


/***********************************************************************/
/***                        Include Files                            ***/
/***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "sdd_defs.h"
#include "sdd_if.h"

/**************************************************************************
**
** NAME         SDD_GetNextIE
**
** PARAMETERS:  Pointer to current IE, Length remainng includign current IE
**
** RETURNS:     Pointer to next IE and updates length remianing
**
** DESCRIPTION : Returns the next IE in the section or NULL
**************************************************************************/

SDD_GENERAL_HEADER_ELT *SDD_GetNextIE(SDD_GENERAL_HEADER_ELT *pIE, uint32_t *pLength)
{
    int ieLength;
    int lengthLeft;

    ieLength = pIE->Length;
    lengthLeft = *pLength - ieLength;

    /* If current IE takes us to the end, end here */
    if (lengthLeft <= 0)
        return NULL;

    /* Move pointer to next IE - Byte wise */
    pIE = (SDD_GENERAL_HEADER_ELT *)((uint8_t *)pIE + ieLength+2);

    /* Check for end of section IE */
    if (pIE->Type == SDD_END_OF_CONTENT_ELT_ID)
        return NULL;

    *pLength = lengthLeft;
    return pIE;
}

/**************************************************************************
**
** NAME         SDD_WriteSection
**
** PARAMETERS:  pIE:   Pointer to current Section
**              pWriteSectIes: Callback for write Section IE by user
**
** RETURNS:     Pointer to next Section
**
** DESCRIPTION : Write a new Section
**************************************************************************/
#include <arpa/inet.h>

uint8_t *SDD_WriteSection(uint8_t *pIE, int sectionId, uint8_t *(pWriteSectIes)(uint8_t *pIE))
{
#define SDD_END_OF_CONTENT_ELT_SIZE 4    /* type + len + pad[2] = 4, IEs are already aligned */
#define SDD_DEFAULT_SECT_MAJOR_VERSION 1
#define SDD_DEFAULT_SECT_MINOR_VERSION 0

    SDD_SECTION_HEADER_ELT hdr = {SDD_SECTION_HEADER_ELT_ID, 6, sectionId, SDD_DEFAULT_SECT_MAJOR_VERSION,
                                  SDD_DEFAULT_SECT_MINOR_VERSION, 0, 0};
    SDD_END_OF_CONTENT_ELT end = {SDD_END_OF_CONTENT_ELT_ID, 2, {0}};

    uint8_t *pStartIes = pIE + sizeof(hdr);
    /* call to user function to write IEs */
    uint8_t *pEndIes = pWriteSectIes(pStartIes);
    /* Add length of SDD_SECTION_HEADER_ELT_ID and SDD_END_OF_CONTENT_ELT_ID */
    hdr.SectionLength = (pEndIes - pStartIes) + sizeof(hdr) + SDD_END_OF_CONTENT_ELT_SIZE; /* not sizeof(end) */
    memcpy(pIE, &hdr, sizeof(hdr));
    memcpy(pEndIes, &end, SDD_END_OF_CONTENT_ELT_SIZE);

    return pEndIes + SDD_END_OF_CONTENT_ELT_SIZE;
}

/**************************************************************************
**
** NAME         SDD_WriteSectIe
**
** PARAMETERS:  pIE:   Pointer to current IE
**              type:  Element ID
**              len:   data length
**              pData: Pointer to element data
**
** RETURNS:     Pointer to next IE
**
** DESCRIPTION : Write a Element
**************************************************************************/
uint8_t *SDD_WriteSectIe(uint8_t *pIE, int type, int len, uint8_t *pData)
{
#define SDD_HDR_SIZE 2

    uint8_t size = len;

    /* Round to next nearest padding but including */
    len += SDD_HDR_SIZE;
    len = ROUND_UP_TO_IE(len);
    len -= SDD_HDR_SIZE;

    pIE[0] = (uint8_t)type;
    pIE[1] = (uint8_t)len;
    memcpy(pIE + SDD_HDR_SIZE, pData, size);
    memset(pIE + SDD_HDR_SIZE + size, 0, len - size);    /* padding */

    return pIE + SDD_HDR_SIZE + len;
}

/**************************************************************************
**
** NAME         SDD_RegenerateLastSection
**
** PARAMETERS:  pIE:   Pointer to current Section
**
** RETURNS:     Pointer to SDD end
**
** DESCRIPTION : Write a last Section which mean the SDD end
**************************************************************************/
uint8_t *SDD_RegenerateLastSection(uint8_t *pIE)
{
    pIE[0] = SDD_SECTION_HEADER_ELT_ID;
    pIE[1] = 6;
    pIE[2] = SDD_LAST_SECT_ID;
    pIE[3] = 0;
    pIE[4] = 0;
    pIE[5] = 0;
    pIE[6] = 0;
    pIE[7] = 0;

    return pIE + SDD_LAST_SECTION_SIZE;
}

/**************************************************************************
**
** NAME         SDD_FindSection
**
** PARAMETERS:  pSDD:   Pointer to SDD data
**              id: The Seciont ID which is wanted to Find
**
** RETURNS:     Pointer to the Section wanted
**
** DESCRIPTION : Find a Section
**************************************************************************/
uint8_t *SDD_FindSection(uint8_t *pSDD, uint8_t id)
{
    SDD_GENERAL_HEADER_ELT  *pIE;
    SDD_SECTION_HEADER_ELT  *pSectIE;

    while (1) {
        pIE = (SDD_GENERAL_HEADER_ELT *)pSDD;
        switch (pIE->Type) {
            case SDD_VERSION_ELT_ID:
            default:
                /* manage IE's that grow and IE's that are unrecognised by skipping */
                pSDD += pIE->Length + SDD_HDR_SIZE;
                break;

            case SDD_SECTION_HEADER_ELT_ID:
                pSectIE = (SDD_SECTION_HEADER_ELT *)pIE;
                if (pSectIE->SectionId == id)
                    return (uint8_t *)pSectIE;
                if (pSectIE->SectionId == SDD_LAST_SECT_ID)
                    return NULL;
                /* length includes section header itself */
                pSDD += pSectIE->SectionLength;
                break;
        }
    }
}
