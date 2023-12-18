/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseSDT.c
* Description : parse the SDT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgram.h"
#include "DVBCoreDescriptor.h"
#include "DVBCoreParseSDT.h"
#include "DVBCoreMemory.h"
/*********************************************************************************************************************************
* Function Name : InitialTsSdtService
* Description : initial the TS_SDT_SERVICE
* Parameters :
* pSdtService -- the TS_SDT_SERVICE
* Returns : void
*********************************************************************************************************************************/
static void InitialTsSdtService(TS_SDT_SERVICE *pSdtService)
{

	pSdtService->service_id = 0x0000;
	pSdtService->reserved_future_use = 0x00;
	pSdtService->EIT_present_following_flag = 0x0;
	pSdtService->EIT_schedule_flag = 0x0;
	pSdtService->running_status = 0x00;
	pSdtService->free_CA_mode = 0x0;
	pSdtService->descriptors_loop_length = 0x0000;
	memset(pSdtService->descriptor, '\0', DESCRIPTOR_MAX_LENGTH_4096);
	pSdtService->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInTsSdtServiceLinkList
* Description : put the TS_SDT_SERVICE to link list
* Parameters :
* pSdtServiceHead -- the link list header
* pSdtService -- the TS_SDT_SERVICE we should put
* Returns : void
*********************************************************************************************************************************/
static void PutInTsSdtServiceLinkList(TS_SDT_SERVICE **pSdtServiceHead, TS_SDT_SERVICE *pSdtService)
{

	TS_SDT_SERVICE *pSdtServiceTemp = *pSdtServiceHead;

	if (NULL == *pSdtServiceHead)
	{
		*pSdtServiceHead = pSdtService;
	}
	else
	{
		while (NULL != pSdtServiceTemp->next)
		{
			pSdtServiceTemp = pSdtServiceTemp->next;
		}
		pSdtServiceTemp->next = pSdtService;
	}
}

/*********************************************************************************************************************************
* Function Name : FreeTsSdtServiceLinkList
* Description : free the memory of the service
* Parameters :
* pSdtService -- the service first address
* Returns : void
*********************************************************************************************************************************/
static void FreeTsSdtServiceLinkList(TS_SDT_SERVICE **pSdtService)
{

	TS_SDT_SERVICE *pSdtServiceTemp = NULL;

	while (NULL != *pSdtService)
	{
		pSdtServiceTemp = (*pSdtService)->next;
		DVBCoreFree(*pSdtService);
		*pSdtService = pSdtServiceTemp;
	}
	*pSdtService = NULL;

}

/*********************************************************************************************************************************
* Function Name : ParseSDT
* Description : parse the SDT table
* Parameters :
* pTS_SDT -- the SDT
* p -- the buffer of PMT section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
static int ParseSDT(TS_SDT *pSDT, unsigned char *p, unsigned char* pVersionNumber)
{

	int nLen = 0;
	int nPos = 11;
	TS_SDT_SERVICE * pSdtServiceMalloc = NULL;

	pSDT->pSdtServiceHead = NULL;
	pSDT->table_id = p[0];
	if(p[0] != 0x42)
	{
		printf("other transport stream");
		return 0;
	}
	pSDT->section_syntax_indicator = p[1] >> 7;
	pSDT->reserved_future_use_1 = (p[1] >> 6) & 0x01;
	pSDT->reserved_1 = (p[1] >> 4) & 0x03;
	pSDT->section_length = ((p[1] & 0x0F) << 8) | p[2];
	pSDT->transport_stream_id = (p[3] << 8) | p[4];
	pSDT->reserved_2 = p[5] >> 6;
	pSDT->version_number = (p[5] >> 1) & 0x1F;
	pSDT->current_next_indicator = (p[5] << 7) >> 7;
    if(pSDT->version_number == *pVersionNumber || !pSDT->current_next_indicator)
    {
		return 0;
    }
	pSDT->section_number = p[6];
	pSDT->last_section_number = p[7];
	pSDT->original_network_id = (p[8] << 8) | p[9];
	pSDT->reserved_future_use_2 = p[10];
	nLen = pSDT->section_length + 3;
	pSDT->CRC_32 = (p[nLen - 4] << 24) 
						| (p[nLen - 3] << 16)
						| (p[nLen - 2] << 8) 
						| p[nLen - 1];
	
	for (nPos = 11; nPos < nLen - 4; nPos += 5)
	{

		pSdtServiceMalloc = (TS_SDT_SERVICE *)DVBCoreMalloc(sizeof(TS_SDT_SERVICE));
		if (NULL == pSdtServiceMalloc)
		{
			printf("sdt service DVBCoreMalloc failed.");
			return -1;
		}
		InitialTsSdtService(pSdtServiceMalloc);
		pSdtServiceMalloc->service_id = (p[nPos] << 8) | p[nPos + 1];
		pSdtServiceMalloc->reserved_future_use = p[nPos + 2] >> 2;
		pSdtServiceMalloc->EIT_schedule_flag = (p[nPos + 2] & 0x03) >> 1;
		pSdtServiceMalloc->EIT_present_following_flag = p[nPos + 2] & 0x01;
		pSdtServiceMalloc->running_status = p[nPos + 3] >> 5;
		pSdtServiceMalloc->free_CA_mode = (p[nPos + 3] & 0x1F) >> 4;

		pSdtServiceMalloc->descriptors_loop_length = ((p[nPos + 3] & 0x0F) << 8)| p[nPos + 4];
		
		if (pSdtServiceMalloc->descriptors_loop_length != 0)
		{
			memcpy(pSdtServiceMalloc->descriptor, p + 5 + nPos,
			pSdtServiceMalloc->descriptors_loop_length);
			pSdtServiceMalloc->descriptor[pSdtServiceMalloc->descriptors_loop_length] = '\0';
			nPos += pSdtServiceMalloc->descriptors_loop_length;
		}
		PutInTsSdtServiceLinkList(&(pSDT->pSdtServiceHead), pSdtServiceMalloc);

	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : PutServiceInformationToProgramList
* Description : put the information(program_name) we get from SDT in the program structure
* Parameters :
* pProgram -- the program (we put information in it)
* pService -- the SDT service
* Returns : void
*********************************************************************************************************************************/
static void PutServiceInformationToProgramList(PROGRAM **pProgram, TS_SDT_SERVICE *pService)
{

	PROGRAM *pTemp = *pProgram;
	int iServiceNamePosition = 0;
	int iServiceDespritorPosition = -1;
	SERVICE_DESCRIPTOR Descriptor = {0};
	while (NULL != pTemp)
	{		
		if (pService->service_id == pTemp->uProgramNumber)
		{
			pTemp->EIT_present_following_flag = pService->EIT_present_following_flag;
			pTemp->EIT_schedule_flag = pService->EIT_schedule_flag;
			pTemp->nFreeCAMode = pService->free_CA_mode;
			if (0 == GetServiceDescriptor(&Descriptor, pService->descriptor, pService->descriptors_loop_length))
			{
				strcpy(pTemp->pucProgramName, Descriptor.service_name);	
				printf("pucProgramName: %s, nFreeCAMode: %d", pTemp->pucProgramName, 
					pTemp->nFreeCAMode);
			}
			break;
		}
		pTemp = pTemp->next;
	}

}

/*********************************************************************************************************************************
* Function Name : PutSdtInformationToProgramList
* Description : put the information(program_name) we get from SDT in the program structure
* Parameters :
* pInProgram -- the program (we put information in it)
* pService -- the SDT service
* Returns : void
*********************************************************************************************************************************/
static void PutSdtInformationToProgramList(TS_SDT *pSDT, PROGRAM **pProgram)
{

	TS_SDT_SERVICE *pService = NULL;
	pService = pSDT->pSdtServiceHead;
	
	while (NULL != pService)
	{
		PutServiceInformationToProgramList(pProgram, pService);
		pService = pService->next;
	}

}

/*********************************************************************************************************************************
* Function Name : ParseFromSDT
* Description : parse the SDT table and put the information in the program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromSDT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber)
{

	TS_SDT mTsSDT = {0};
	
	if (-1 == ParseSDT(&mTsSDT, p, pVersionNumber))
	{
		printf("ParseSDT error!\n");
		return -1;
	}
	PutSdtInformationToProgramList(&mTsSDT, pProgram);
	FreeTsSdtServiceLinkList(&(mTsSDT.pSdtServiceHead));
	return 0;

}

