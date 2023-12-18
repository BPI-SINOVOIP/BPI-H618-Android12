/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParsePAT.c
* Description : parse the PAT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgram.h"
#include "DVBCoreParsePAT.h"
#include "DVBCoreMemory.h"
/*********************************************************************************************************************************
* Function Name : InitialTsPatProgram
* Description : initial the TS_PAT_PROGRAM
* Parameters :
* pPatProgram -- the TS_PAT_PROGRAM
* Returns : void
*********************************************************************************************************************************/
static void InitialTsPatProgram(TS_PAT_PROGRAM *pPatProgram)
{

	pPatProgram->program_number = 0x0000;
	pPatProgram->program_map_PID = 0x0000;
	pPatProgram->reserved_3 = 0x0;
	pPatProgram->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInTsPatProgramLinkList
* Description : put the TS_PAT_PROGRAM to link list
* Parameters :
* pPatProgramHead -- the TS_PAT_PROGRAM link list header
* pPatProgram -- the TS_PAT_PROGRAM which we should put in the link list
* Returns : void
*********************************************************************************************************************************/
static void PutInTsPatProgramLinkList(TS_PAT_PROGRAM **pPatProgramHead, TS_PAT_PROGRAM *pPatProgram)
{

	TS_PAT_PROGRAM *pPatProgramTemp = *pPatProgramHead;	
	if (NULL == (*pPatProgramHead))
	{
		(*pPatProgramHead) = pPatProgram;
	}
	else
	{
		while (NULL != pPatProgramTemp->next)
		{
			pPatProgramTemp = pPatProgramTemp->next;
		}
		pPatProgramTemp->next = pPatProgram;
	}

}

/*********************************************************************************************************************************
* Function Name : FreeTsPatProgramLinkList
* Description : free the memory of the TS_PAT_PROGRAM link list
* Parameters :
* pPatProgram -- the head of the link list
* Returns : void
*********************************************************************************************************************************/
static void FreeTsPatProgramLinkList(TS_PAT_PROGRAM **pPatProgram)
{

	TS_PAT_PROGRAM *pPatProgramTemp = NULL;

	while (NULL != (*pPatProgram))
	{
		pPatProgramTemp = (*pPatProgram)->next;
		DVBCoreFree(*pPatProgram);
		(*pPatProgram) = pPatProgramTemp;
	}
	(*pPatProgram) = NULL;

}

/*********************************************************************************************************************************
* Function Name : ParsePAT
* Description : parse the pat table
* Parameters :
* pPAT -- the TS_PAT
* p -- the PAT section buffer
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
static int ParsePAT(TS_PAT *pPAT, unsigned char *p, unsigned char* pVersionNumber)
{

	int nLen = 0;
	int nPos = 8;
	TS_PAT_PROGRAM *pPatMalloc = NULL;
	pPAT->pPatProgramHead = NULL;
	pPAT->table_id = p[0];

	if(pPAT->table_id != 0x00u)
    {
    	printf("should not be here.\n");
		return -1;
    }

	
	pPAT->section_syntax_indicator = p[1] >> 7;
	pPAT->zero = (p[1] >> 6) & 0x1;
	pPAT->reserved_1 = (p[1] >> 4) & 0x3;
	pPAT->section_length = ((p[1] & 0x0F) << 8) | p[2];
	pPAT->transport_stream_id = (p[3] << 8) | p[4];
	pPAT->reserved_2 = p[5] >> 6;

	pPAT->version_number = (p[5] >> 1) & 0x1F;
	pPAT->current_next_indicator = (p[5] << 7) >> 7;

    if(pPAT->version_number == *pVersionNumber || !pPAT->current_next_indicator)
    {
		return 0;
    }
	//printf("version_number: %d, pPAT->version_number: %d\n", *pVersionNumber, pPAT->version_number);
	*pVersionNumber = pPAT->version_number;
	
	pPAT->section_number = p[6];
	pPAT->last_section_number = p[7];
	nLen = 3 + pPAT->section_length;
	pPAT->CRC_32 = (p[nLen - 4] << 24) | (p[nLen - 3] << 16)| (p[nLen - 2] << 8) | p[nLen - 1];

	/*----------------------------------------------------------*/
	/*get the TS_PAT_PROGRAM and put in linked list */
	/*----------------------------------------------------------*/
	for (nPos = 8; nPos < nLen - 4; nPos += 4)
	{
		if (0x00 == ((p[nPos] << 8) | p[1 + nPos])) //NIT
		{
			pPAT->network_PID = ((p[2 + nPos] & 0x1F) << 8) | p[3 + nPos];
			continue;
			//printf("the network_PID %0x\n", pPAT->network_PID);
		}
		pPatMalloc = (TS_PAT_PROGRAM *)DVBCoreMalloc(sizeof(TS_PAT_PROGRAM));
		if (NULL == pPatMalloc)
		{
			printf("PAT DVBCoreMalloc failed.");
			return -1;

		}		
		InitialTsPatProgram(pPatMalloc);
		pPatMalloc->program_map_PID = ((p[2 + nPos] & 0x1F) << 8) | p[3 + nPos];
		pPatMalloc->program_number = (p[nPos] << 8) | p[1 + nPos];
		pPatMalloc->reserved_3 = p[2 + nPos] >> 5;
		
		PutInTsPatProgramLinkList(&(pPAT->pPatProgramHead), pPatMalloc);

	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : PutPatInfomationToProgramList
* Description : put the PAT information to the PROGRAM link list
* Parameters :
* pPAT -- the TS_PAT
* pProgram -- the program where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
static int PutPatInfomationToProgramList(TS_PAT *pPAT, PROGRAM **pProgram)
{

	PROGRAM *pProgramMalloc = NULL;
	TS_PAT_PROGRAM *pPatProgramTemp = NULL;

	pPatProgramTemp = pPAT->pPatProgramHead;
	while (NULL != pPatProgramTemp)
	{

		pProgramMalloc = (PROGRAM *)DVBCoreMalloc(sizeof(PROGRAM)); //apply the space of the program
		if (NULL == pProgramMalloc)
		{
			printf("program DVBCoreMalloc failed.");
			return -1;

		}
		InitialProgram(pProgramMalloc);
		pProgramMalloc->uProgramNumber = pPatProgramTemp->program_number;
		pProgramMalloc->uPMT_PID = pPatProgramTemp->program_map_PID;
		
		PutInProgramLinkList(pProgram, pProgramMalloc);
		pPatProgramTemp = pPatProgramTemp->next;

	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : ParseFromPAT
* Description : parse the PAT table and put the information in PROGRAM link list
* Parameters :
* p -- the buffer of section
* pProgram -- the program where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromPAT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber)
{

	TS_PAT mTsPAT = {0};
	if (-1 == ParsePAT(&mTsPAT, p, pVersionNumber)) //parse PAT and get the information
	{
		printf("parse PAT failed");
		return -1;
	}
	
	/*----------------------------------------------------------*/
	/* deal with the PAT information */
	/*----------------------------------------------------------*/
	if (-1 == PutPatInfomationToProgramList(&mTsPAT, pProgram))
	{
		return -1;
	}
	FreeTsPatProgramLinkList(&(mTsPAT.pPatProgramHead));
	return 0;

}

