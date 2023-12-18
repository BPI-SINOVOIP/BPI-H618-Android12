/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : Program.c
* Description : some actions for program information
* Version : 1.0
* History :
* xhw : 2015-9-2 Create
*********************************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "DVBCoreProgram.h"
#include "DVBCoreMemory.h"
#define DEFAULT_SERVICE_NAME    "NoName"
/*********************************************************************************************************************************
* Function Name : InitialProgram
* Description : initial program
* Parameters :
* pProgram -- the initial program
* Returns : void
*********************************************************************************************************************************/
void InitialProgram(PROGRAM *pProgram)
{
	int i;
	pProgram->uProgramNumber = 0x0000;
	pProgram->uPMT_PID = 0x0000;
	memset(pProgram->pucProgramName, '\0', DC_NAME_LENGTH);
	memcpy(pProgram->pucProgramName, DEFAULT_SERVICE_NAME, strlen(DEFAULT_SERVICE_NAME));
	memset(pProgram->aucStartTime, '\0', 25);
	for(i=0;i<MAX_AUDIO_NUM;i++)
	{
		pProgram->uAudioPID[i] = 0x1fff;
	}	

	pProgram->uPCR_PID = 0x000;
	pProgram->uVideoPID = 0x000;	
	pProgram->nFreeCAMode = 0x0;	
	pProgram->CA_system_ID = 0x0000;
	pProgram->ECM_PID = 0x0000;	
	pProgram->EMM_PID = 0x0000;
	pProgram->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInProgramLinkList
* Description : put the program to link list
* Parameters :
* pProgramHead -- the link list header
* pProgram -- the program
* Returns : void
*********************************************************************************************************************************/
void PutInProgramLinkList(PROGRAM **pProgramHead, PROGRAM *pProgram)
{

	PROGRAM *pProgramTemp = *pProgramHead;

	if (NULL == *pProgramHead)
	{

		*pProgramHead = pProgram;

	}
	else
	{

		while (NULL != pProgramTemp->next)
		{

			pProgramTemp = pProgramTemp->next;

		}
		pProgramTemp->next = pProgram;

	}

}

/*********************************************************************************************************************************
* Function Name : GetProgramInformation
* Description : out put all of the program information
* Parameters :
* pProgram -- the program
* Returns : void
*********************************************************************************************************************************/
void GetProgramInformation(PROGRAM *pProgram)
{

	PROGRAM *pTempp = pProgram;

	while (NULL != pTempp)
	{

		if (0x00 != pTempp->uProgramNumber)
		{

			printf("%4x %4x %25s %4x %4x\n", pTempp->uProgramNumber, pTempp->uPMT_PID,
			pTempp->pucProgramName, pTempp->uPCR_PID, pTempp->uVideoPID);

		}
		pTempp = pTempp->next;

	}

}

/*********************************************************************************************************************************
* Function Name : FreeProgramMemory
* Description : free the memory of program
* Parameters :
* pProgram -- the program which we should free memory
* Returns : void
*********************************************************************************************************************************/
void FreeProgramMemory(PROGRAM *pProgram)
{

	PROGRAM *pTemp = NULL;

	while (NULL != pProgram)
	{

		pTemp = pProgram;
		pProgram = pProgram->next;
		DVBCoreFree(pTemp);
		pTemp = NULL;

	}

}
