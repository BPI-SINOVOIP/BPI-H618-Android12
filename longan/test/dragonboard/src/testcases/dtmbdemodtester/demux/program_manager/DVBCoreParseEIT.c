/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseEIT.c
* Description : parse the SDT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgramEvent.h"
#include "DVBCoreDescriptor.h"
#include "DVBCoreParseEIT.h"
#include "DVBCoreMemory.h"
/*********************************************************************************************************************************
* Function Name : InitialTsEitEvent
* Description : initial the TS_EIT_EVENT
* Parameters :
* pEitEvent -- the TS_EIT_EVENT
* Returns : void
*********************************************************************************************************************************/
static void InitialTsEitEvent(TS_EIT_EVENT *pEitEvent)
{
	pEitEvent->service_id = 0x0000;
	pEitEvent->event_id = 0x0000;
	memset(pEitEvent->start_time, '\0', 5);
	pEitEvent->duration = 0x0000;
	pEitEvent->running_status = 0x00;
	pEitEvent->free_CA_mode = 0x0;
	pEitEvent->descriptors_loop_length = 0x0000;
	memset(pEitEvent->descriptor, '\0', DESCRIPTOR_MAX_LENGTH_4096);
	pEitEvent->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInTsEitEventLinkList
* Description : put the event to link list
* Parameters :
* pEitEventHead -- the link list header
* pEitEvent -- the PAT program we should put
* Returns : void
*********************************************************************************************************************************/
static void PutInTsEitEventLinkList(TS_EIT_EVENT **pEitEventHead, TS_EIT_EVENT *pEitEvent)
{

	TS_EIT_EVENT *pEitEventTemp = *pEitEventHead;

	if (NULL == *pEitEventHead)
	{
		*pEitEventHead = pEitEvent;
	}
	else
	{
		while (NULL != pEitEventTemp->next)
		{
			pEitEventTemp = pEitEventTemp->next;
		}
		pEitEventTemp->next = pEitEvent;

	}

}

/*********************************************************************************************************************************
* Function Name : FreeTsEitEventLinkList
* Description : free the memory of the event
* Parameters :
* pEitEvent -- the stream first address
* Returns : void
*********************************************************************************************************************************/
static void FreeTsEitEventLinkList(TS_EIT_EVENT **pEitEvent)
{

	TS_EIT_EVENT *pEitEventTemp = NULL;

	while (NULL != *pEitEvent)
	{
		pEitEventTemp = (*pEitEvent)->next;
		DVBCoreFree(*pEitEvent);
		*pEitEvent = pEitEventTemp;
	}
	*pEitEvent = NULL;

}

/*********************************************************************************************************************************
* Function Name : ParseEIT
* Description : parse the EIT table
* Parameters :
* pTS_EIT -- the EIT
* pucBuffer -- the buffer of EIT section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
static int ParseEIT(TS_EIT *pEIT, unsigned char *p, unsigned char* pVersionNumber, SCH_SEC_MASK *pMask, unsigned int pro_num, unsigned int *event_cnt)
{

	int nLen = 0;
	int nPos = 0;
    int nEventCount = 0;
	TS_EIT_EVENT * pEitMalloc = NULL;

	pEIT->pEitEventHead = NULL;
	pEIT->table_id = p[0];
	if(!((pEIT->table_id == 0x4E) || ((pEIT->table_id & 0xF0) == 0x50)))
	{
		return 0;
	}

	if((pEIT->table_id & 0xF0) == 0x50)
	{
		if((pEIT->table_id & 0x0F) >= EIT_SCHEDULE_TABLE_ID_NUM)
		{
			printf("schedule table id is overflow!!");
			return 0;
		}		
		pEIT->version_number = (p[5] >> 1) & 0x1F;
		pEIT->current_next_indicator = (p[5] << 7) >> 7;
		pEIT->section_number = p[6];
	    if(pEIT->version_number != *pVersionNumber
			&& *pVersionNumber != 0xff
			&& pEIT->current_next_indicator == 1)
	    {
			printf("schdl:version number unchanged.%d %d", *pVersionNumber, pEIT->version_number);		
			FreeTsEitEventLinkList(&(pEIT->pEitEventHead));
			*pVersionNumber = 0xff;
			pMask->mask = 0;
	    }
		
		pEIT->section_syntax_indicator = p[1] >> 7;
		pEIT->reserved_future_use_1 = (p[1] >> 6) & 0x01;
		pEIT->reserved_1 = (p[1] >> 4) & 0x03;
		pEIT->section_length = ((p[1] & 0x0F)) << 8 | p[2];
		pEIT->service_id = (p[3] << 8) | p[4];
		pEIT->reserved_2 = p[5] >> 6;
		pEIT->last_section_number = p[7];
		pEIT->transport_stream_id = (p[8] << 8) | p[9];
		pEIT->original_network_id = (p[10] << 8) | p[11];
		pEIT->segment_last_section_number = p[12];
		pEIT->last_table_id = p[13];
		nLen = pEIT->section_length + 3;
		pEIT->CRC_32 = (p[nLen - 4] << 24) | (p[nLen - 3] << 16) | (p[nLen - 2] << 8) | p[nLen - 1];

		//logv("last_segment: %d", pEIT->last_section_number>>3);
		if(pEIT->section_number != pEIT->segment_last_section_number)
		{
			printf("this segment has serval sections!");
			printf("schdl:section_number: %d, last_section_number: %d segment_last_section_number: %d",
				pEIT->section_number, pEIT->last_section_number, pEIT->segment_last_section_number);
		}
		//logv("(%d)schdl: section_number: %d ser_id: %d section_number: %d mask: %d", pro_num, pEIT->section_number, 
			//pEIT->service_id, pEIT->section_number, pMask->mask);		
		if ((pMask->mask & (1<<(pEIT->section_number & 0x7))) != 0)
		{
			//printf("schdl:(%x)section %dth already exist!", pEIT->table_id, pEIT->section_number);
			return 0;
		}

		for (nPos = 14; nPos < nLen - 4; nPos += 12)
		{		
			pEitMalloc = (TS_EIT_EVENT *)DVBCoreMalloc(sizeof(TS_EIT_EVENT));
			if (NULL == pEitMalloc)
			{
				printf("schdl:EIT DVBCoreMalloc failed.");
				return -1;
			}		
			InitialTsEitEvent(pEitMalloc);
			pEitMalloc->service_id = pEIT->service_id;
			pEitMalloc->event_id = (p[nPos]) << 8 | p[nPos + 1];
			memcpy(pEitMalloc->start_time, p + nPos + 2, 5);
			pEitMalloc->duration = (p[nPos + 7] << 16) | (p[nPos + 8] << 8) | (p[nPos + 9]);
			pEitMalloc->running_status = p[nPos + 10] >> 5;
			pEitMalloc->free_CA_mode = (p[nPos + 10] & 0x1F) >> 4;
			pEitMalloc->descriptors_loop_length = ((p[nPos + 10] & 0x0F) << 8) | p[nPos + 11];
			if (pEitMalloc->descriptors_loop_length != 0)
			{
				memcpy(pEitMalloc->descriptor, p + 12 + nPos, pEitMalloc->descriptors_loop_length);
				pEitMalloc->descriptor[pEitMalloc->descriptors_loop_length] = '\0';
				nPos += pEitMalloc->descriptors_loop_length;
			}
			PutInTsEitEventLinkList(&(pEIT->pEitEventHead),pEitMalloc);
            nEventCount++;
		}
		
		pMask->mask|= (1 << (pEIT->section_number & 0x7));
		*pVersionNumber = pEIT->version_number;
        *event_cnt = nEventCount;
		//printf("schdl:(%d)pMask->mask: %d, evnet count %d !", pEIT->section_number, pMask->mask, *event_cnt);
	}
	else
	{
		unsigned int event_info_len = 0;
		pEIT->version_number = (p[5] >> 1) & 0x1F;
		pEIT->current_next_indicator = (p[5] << 7) >> 7;
		pEIT->section_number = p[6];
		event_info_len = pEIT->section_number - (11 + 4);		
		//printf("(%d) version_number: %d %d", pEIT->section_number, 
			//pEIT->version_number, *pVersionNumber);

		if(pEIT->version_number != *pVersionNumber
			&& *pVersionNumber != 0xff
			&& pEIT->current_next_indicator == 1)
	    {
			printf("pf: version number unchanged.%d %d", *pVersionNumber, pEIT->version_number);		
			FreeTsEitEventLinkList(&(pEIT->pEitEventHead));
			*pVersionNumber = 0xff;
			pMask->mask = 0;
	    }
		
		printf("(%d)pf: section_number: %d ser_id: %d section_number: %d mask: %d", pro_num, pEIT->section_number, 
			pEIT->service_id, pEIT->section_number, pMask->mask);
		if (event_info_len == 0
			|| ((1<<pEIT->section_number) & pMask->mask) != 0) //section already exist
		{
			//printf("section %d already exist!\n", pEIT->section_number);
			return 0;
		}
		
		pEIT->section_syntax_indicator = p[1] >> 7;
		pEIT->reserved_future_use_1 = (p[1] >> 6) & 0x01;
		pEIT->reserved_1 = (p[1] >> 4) & 0x03;
		pEIT->section_length = ((p[1] & 0x0F)) << 8 | p[2];
		pEIT->service_id = (p[3] << 8) | p[4];
		pEIT->reserved_2 = p[5] >> 6;
		pEIT->last_section_number = p[7];
		pEIT->transport_stream_id = (p[8] << 8) | p[9];
		pEIT->original_network_id = (p[10] << 8) | p[11];
		pEIT->segment_last_section_number = p[12];
		pEIT->last_table_id = p[13];
		nLen = pEIT->section_length + 3;
		pEIT->CRC_32 = (p[nLen - 4] << 24) | (p[nLen - 3] << 16) | (p[nLen - 2] << 8) | p[nLen - 1];
				
		for (nPos = 14; nPos < nLen - 4; nPos += 12)
		{		
			pEitMalloc = (TS_EIT_EVENT *)DVBCoreMalloc(sizeof(TS_EIT_EVENT));
			if (NULL == pEitMalloc)
			{
				printf("pf: EIT DVBCoreMalloc failed.");
				return -1;
			}		
			InitialTsEitEvent(pEitMalloc);
			pEitMalloc->service_id = pEIT->service_id;
			pEitMalloc->event_id = (p[nPos]) << 8 | p[nPos + 1];
			memcpy(pEitMalloc->start_time, p + nPos + 2, 5);
			pEitMalloc->duration = (p[nPos + 7] << 16) | (p[nPos + 8] << 8) | (p[nPos + 9]);
			pEitMalloc->running_status = p[nPos + 10] >> 5;
			pEitMalloc->free_CA_mode = (p[nPos + 10] & 0x1F) >> 4;
			pEitMalloc->descriptors_loop_length = ((p[nPos + 10] & 0x0F) << 8) | p[nPos + 11];
			if (pEitMalloc->descriptors_loop_length != 0)
			{
				memcpy(pEitMalloc->descriptor, p + 12 + nPos, pEitMalloc->descriptors_loop_length);
				pEitMalloc->descriptor[pEitMalloc->descriptors_loop_length] = '\0';
				nPos += pEitMalloc->descriptors_loop_length;
			}
			PutInTsEitEventLinkList(&(pEIT->pEitEventHead),pEitMalloc);
            nEventCount++;
		}

		pMask->mask |= (1<<pEIT->section_number);
		*pVersionNumber = pEIT->version_number;	
        *event_cnt = nEventCount;
		printf("pf: (%d)pMask->mask: %d,event count %d !", pEIT->section_number, pMask->mask, *event_cnt);
	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : GetTheEventStartTime
* Description : out put the start time
* Parameters :
* pucTime -- the buffer of time
* Returns : void
*********************************************************************************************************************************/
static void GetTheEventStartTime(unsigned char *pucTime, char *aucStartTime)
{

	int iDay = 0;
	int iTemp = 0;
	int iYear = 0;
	int iMonth = 0;
	unsigned int uiMJD = 0;
	if (NULL != pucTime)
	{
		uiMJD = (pucTime[0] << 8) | pucTime[1];
		iYear = (int)((uiMJD - 15078.2) / 365.25);
		iMonth = (int)((uiMJD - 14956.1 - (int)(iYear * 365.25)) / 30.6001);
		iDay = (int)(uiMJD - 14956 - (int)(iYear * 365.25) - (int)(iMonth * 30.6001));
		if ((14 == iMonth) || (15 == iMonth))
		{
			iTemp = 1;
		}
		iYear = iYear + iTemp;
		iMonth = iMonth - 1 - iTemp * 12;
		iYear = iYear + 1900;

		sprintf(aucStartTime,"%d-%d-%d %02x:%02x:%02x",iYear,iMonth,iDay,(unsigned char)pucTime[2],(unsigned char)pucTime[3], (unsigned char)pucTime[4]);
		//sprintf(aucStartTime,"%02x:%02x",(unsigned char)pucTime[2],(unsigned char)pucTime[3]);
		//printf("the start time is %s\n",aucStartTime);

	}

}

/*********************************************************************************************************************************
* Function Name : GetTheDurationTime
* Description : out put the duration time
* Parameters :
* uiDuration -- the duration time
* Returns : void
*********************************************************************************************************************************/
static void GetTheDurationTime(unsigned int uiDurationTime, char *aucDurationTime)
{

	sprintf(aucDurationTime,"%02x:%02x:%02x",(unsigned char)(uiDurationTime >> 16),(unsigned char)(uiDurationTime >> 8),(unsigned char)uiDurationTime);
	//printf("the duration stime is %s\n", aucDurationTime);

}

/*********************************************************************************************************************************
* Function Name : GetShortEventInfo
* Description : out put the event information in the descriptor tag 0x4d
* Parameters :
* pucDescriptor -- the descriptor
* uiDescriprotLength -- the descriptor length
* Returns : void
*********************************************************************************************************************************/
static void GetShortEventInfo(unsigned char *pucDescriptor, unsigned int uiDescriprotLength, PROGRAM_EVENT *pEvent)
{

	SHORT_EVENT_DESCRIPTOR Descriptor = {0};

	if (0 == GetShortEventDescriptor(&Descriptor, pucDescriptor, uiDescriprotLength))
	{

		printf("the event_name_char is : %s\n", Descriptor.event_name_char);
		//logv("the text_char is : %s\n", Descriptor.text_char);
		strcpy(pEvent->event_name_char, Descriptor.event_name_char); 
		strcpy(pEvent->text_char, Descriptor.text_char);
		pEvent->language = Descriptor.ISO_639_language_code;

	}

}

/*********************************************************************************************************************************
* Function Name : PutEitInformationToProgramEventList
* Description : put the information(program_name) we get from SDT in the program structure
* Parameters :
* pProgram -- the program (we put information in it)
* pEIT -- the SDT service
* Returns : void
*********************************************************************************************************************************/
static int PutEitInformationToProgramEventList(TS_EIT *pEIT, PROGRAM_EVENT **pProgramEvent)
{

	PROGRAM_EVENT *pProgramEventMalloc = NULL;
	TS_EIT_EVENT *pEitEventTemp = NULL;

	pEitEventTemp = pEIT->pEitEventHead;

	while (NULL != pEitEventTemp)
	{

		pProgramEventMalloc = (PROGRAM_EVENT *)DVBCoreMalloc(sizeof(PROGRAM_EVENT)); //apply the space of the program event
		if (NULL == pProgramEventMalloc)
		{
			printf("program event DVBCoreMalloc failed.");
			return -1;

		}
		InitialProgramEvent(pProgramEventMalloc);
		pProgramEventMalloc->service_id = pEitEventTemp->service_id;
		pProgramEventMalloc->event_id = pEitEventTemp->event_id;
		pProgramEventMalloc->running_status = pEitEventTemp->running_status;
		pProgramEventMalloc->free_CA_mode = pEitEventTemp->free_CA_mode;
		
		GetTheEventStartTime(pEitEventTemp->start_time, pProgramEventMalloc->start_time);
		GetTheDurationTime(pEitEventTemp->duration, pProgramEventMalloc->duration);
		GetShortEventInfo(pEitEventTemp->descriptor, pEitEventTemp->descriptors_loop_length, pProgramEventMalloc);
		PutInProgramEventLinkList(pProgramEvent, pProgramEventMalloc);
		pEitEventTemp = pEitEventTemp->next;
	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : ParseFromEIT
* Description : parse the EIT table and print the current program information
* Parameters :
* p -- the buffer of section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromEIT(unsigned char *p, PROGRAM_EVENT **pProgramEvent, unsigned char* pVersionNumber, SCH_SEC_MASK* pMask, unsigned int pro_num, unsigned int *event_cnt)
{

	TS_EIT mTsEIT = {0};

	if (-1 == ParseEIT(&mTsEIT, p, pVersionNumber, pMask, pro_num, event_cnt))
	{
		printf("Parse EIT error.");
		return -1;
	}
	
	PutEitInformationToProgramEventList(&mTsEIT, pProgramEvent);
	FreeTsEitEventLinkList(&(mTsEIT.pEitEventHead));
	return 0;

}
