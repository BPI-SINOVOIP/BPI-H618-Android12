/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : ProgramEvent.c
* Description : some actions for program information
* Version : 1.0
* History :
* xhw : 2015-9-2 Create
*********************************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "DVBCoreProgramEvent.h"
#include "DVBCoreMemory.h"
/*********************************************************************************************************************************
* Function Name : InitialProgramEvent
* Description : initial program event
* Parameters :
* pProgram -- the initial program event
* Returns : void
*********************************************************************************************************************************/
void InitialProgramEvent(PROGRAM_EVENT *pProgramEvent)
{
	pProgramEvent->event_id = 0x0000;
	memset(pProgramEvent->start_time, '\0', 25);
	memset(pProgramEvent->duration, '\0', 25);
	pProgramEvent->running_status = 0x0;
	pProgramEvent->free_CA_mode = 0x0;
	pProgramEvent->language = 0x000;
	memset(pProgramEvent->event_name_char, '\0', DC_NAME_LENGTH);
	memset(pProgramEvent->text_char, '\0', DC_NAME_LENGTH);
	pProgramEvent->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInProgramEventLinkList
* Description : put the program event to link list
* Parameters :
* pProgramHead -- the link list header
* pProgramEvent -- the program event
* Returns : void
*********************************************************************************************************************************/
void PutInProgramEventLinkList(PROGRAM_EVENT **pProgramHead, PROGRAM_EVENT *pProgramEvent)
{

	PROGRAM_EVENT *pProgramEventTemp = *pProgramHead;

	if (NULL == *pProgramHead)
	{

		*pProgramHead = pProgramEvent;

	}
	else
	{

		while (NULL != pProgramEventTemp->next)
		{

			pProgramEventTemp = pProgramEventTemp->next;

		}
		pProgramEventTemp->next = pProgramEvent;

	}

}

/*********************************************************************************************************************************
* Function Name : GetProgramEventInformation
* Description : out put all of the program information
* Parameters :
* pProgramEvent -- the program Event
* Returns : void
*********************************************************************************************************************************/
void GetProgramEventInformation(PROGRAM_EVENT *pProgramEvent)
{

	PROGRAM_EVENT *pTempp = pProgramEvent;

	while (NULL != pTempp)
	{
		
		pTempp = pTempp->next;

	}

}

/*********************************************************************************************************************************
* Function Name : FreeProgramEventMemory
* Description : free the memory of program event
* Parameters :
* pProgramEvent -- the program event which we should free memory
* Returns : void
*********************************************************************************************************************************/
void FreeProgramEventMemory(PROGRAM_EVENT *pProgramEvent)
{

	PROGRAM_EVENT *pTemp = NULL;

	while (NULL != pProgramEvent)
	{

		pTemp = pProgramEvent;
		pProgramEvent = pProgramEvent->next;
		DVBCoreFree(pTemp);
		pTemp = NULL;
	}

}
