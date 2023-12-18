/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseCAT.c
* Description : parse the CAT
* Version : 1.0
* History :
* xhw : 2015-9-15 Create
*********************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgram.h"
#include "DVBCoreDescriptor.h"
#include "DVBCoreParseCAT.h"
/*********************************************************************************************************************************
* Function Name : ParseCAT
* Description : parse the CAT table
* Parameters :
* pCAT -- the CAT
* p -- the buffer of CAT section
* Returns : void
*********************************************************************************************************************************/
static int ParseCAT(TS_CAT *pCAT, unsigned char *p, unsigned char* pVersionNumber)
{

	int nLen = 0;
	int nDesLen = 0;

	pCAT->table_id = p[0];
	pCAT->section_syntax_indicator = p[1] >> 7;
	pCAT->zero = (p[1] >> 6) & 0x01;
	pCAT->reserved_1 = (p[1] >> 4) & 0x03;
	pCAT->section_length = ((p[1] & 0x0F) << 8) | p[2];
	pCAT->reserved_2 = (p[3] << 10) | (p[4] << 2) | (p[5] >> 6);
	pCAT->version_number = (p[5] >> 1) & 0x1F;	
	pCAT->current_next_indicator = (p[5] << 7) >> 7;
    if(pCAT->version_number == *pVersionNumber || !pCAT->current_next_indicator)
    {
		return 0;
    }
	pCAT->section_number = p[6];
	pCAT->last_section_number = p[7];
	nLen = pCAT->section_length + 3;
	nDesLen = nLen - 8 - 4;
	memcpy(pCAT->descriptor, p + 8, nDesLen);
	pCAT->descriptor[nDesLen] = '\0';
	pCAT->CRC_32 = (p[nLen - 4] << 24) | (p[nLen - 3] << 16)| (p[nLen - 2] << 8) | p[nLen - 1];
	return 0;

}

/*********************************************************************************************************************************
* Function Name : GetCatInformation
* Description : Get the CAT information
* Parameters :
* pCAT -- the TS_CAT
* pProgram -- the PROGRAM
* Returns : void
*********************************************************************************************************************************/
static void GetCatInformation(TS_CAT *pCAT, PROGRAM **pProgram)
{

	unsigned int nDesLen = 0;
	PROGRAM *pProgramTemp = *pProgram;
	CA_IDENTIFIER_DESCRIPTOR Descriptor = {0}; // for CAT table get the EMM_PID

	nDesLen = pCAT->section_length - 8 - 4;

	if (0 == GetCaIdentifierDescriptor(&Descriptor, pCAT->descriptor, nDesLen))
	{
		printf("the EMM PID is: %x\n", Descriptor.ca_pid);
	}
	
	/*----------------------------------------------------------*/
	/* Get the ECM PID information get from the PMT table */
	/*----------------------------------------------------------*/
	while (NULL != pProgramTemp)
	{
		if (0 != pProgramTemp->ECM_PID)
		{
			printf("the program number: %x the ECM PID :%x\n", pProgramTemp->uProgramNumber, pProgramTemp->ECM_PID);
		}
		pProgramTemp = pProgramTemp->next;
	}

}

/*********************************************************************************************************************************
* Function Name : ParseFromCAT
* Description : parse the CAT table and put the information in program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromCAT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber)
{
	int err = 0;
	TS_CAT mTsCAT = {0};
	
	err = ParseCAT(&mTsCAT, p, pVersionNumber); //parse the CAT
	GetCatInformation(&mTsCAT, pProgram);
	return err;

}
