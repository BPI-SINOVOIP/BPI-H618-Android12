/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreProgramEvent.h
* Description: some actions for program event information
* Version: 1.0
* History:
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/

#ifndef _PROGRAM_EVENT_H_
#define _PROGRAM_EVENT_H_
#include <dc_types.h>

/*use EIT_SCHEDULE_TABLE_ID_NUM tables to save epg infos and
  each table saves 32*3 hours program infos at most
*/
#define EIT_SCHEDULE_TABLE_ID_NUM 	2

typedef struct _PROGRAM_EVENT
{
	unsigned int service_id :16;
	unsigned int event_id :16;
	char start_time[25];
	char duration[25];
	unsigned int running_status :3;
	unsigned int free_CA_mode :1;
	unsigned int language :24;
	char event_name_char[DC_NAME_LENGTH];
	char text_char[DC_NAME_LENGTH];
	struct _PROGRAM_EVENT *next;
	
} PROGRAM_EVENT;

typedef struct _SCH_SEC_MASK
{
	unsigned char	mask;
	unsigned int 	service_id;
	
}SCH_SEC_MASK;


/*********************************************************************************************************************************
* Function Name : InitialProgramEvent
* Description: initial program Event
* Parameters:
* pProgramEvent -- the program event which we should free memory
* Returns: void
*********************************************************************************************************************************/
void InitialProgramEvent(PROGRAM_EVENT *pProgramEvent);

/*********************************************************************************************************************************
* Function Name : PutInProgramEventLinkList
* Description: put the program event to link list
* Parameters:
* pProgramHead -- the link list header
* pProgramEvent -- the program event we should put
* Returns: void
*********************************************************************************************************************************/
void PutInProgramEventLinkList(PROGRAM_EVENT **pProgramHead, PROGRAM_EVENT *pProgramEvent);

/*********************************************************************************************************************************
* Function Name : PintProgramEventInformation
* Description: out put all of the program event information
* Parameters:
* pProgramEvent -- the program event
* Returns: void
*********************************************************************************************************************************/
void PintProgramEventInformation(PROGRAM_EVENT *pProgramEvent);

/*********************************************************************************************************************************
* Function Name : FreeProgramEventMemory
* Description: free the memory of program event
* Parameters:
* pProgramEvent -- the program event which we should free memory
* Returns: void
*********************************************************************************************************************************/
void FreeProgramEventMemory(PROGRAM_EVENT *pProgramEvent);

/*********************************************************************************************************************************
* Function Name : DeleteSameProgramEventLinkList
* Description : delete the same program event in LinkList
* Parameters :
* pProgramHead -- the header of LinkList
* Returns : void
*********************************************************************************************************************************/
void DeleteSameProgramEventLinkList(PROGRAM_EVENT **pProgramHead);

#endif /* PROGRAM_EVENT_H_ */
