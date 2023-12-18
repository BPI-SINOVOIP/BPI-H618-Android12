/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseTDT.c
* Description : parse the TDT
* Version : 1.0
* History :
* xhw : 2015-9-15 Create
*********************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgram.h"
#include "DVBCoreDescriptor.h"
#include "DVBCoreParseTDT.h"
/*********************************************************************************************************************************
* Function Name : ParseTDT
* Description : parse the TDT table
* Parameters :
* pTDT -- the TDT
* p -- the buffer of TDT section
* Returns : void
*********************************************************************************************************************************/
static void ParseTDT(TS_TDT *pTDT, unsigned char *p)
{

	int nLen = 0;
	int nDesLen = 0;

	pTDT->table_id = p[0];
	pTDT->section_syntax_indicator = p[1] >> 7;
	pTDT->reserved_future_use = (p[1] >> 6) & 0x01;
	pTDT->reserved_1 = (p[1] >> 4) & 0x03;
	pTDT->section_length = ((p[1] & 0x0F) << 8) | p[2];
	
	memcpy(pTDT->UTC_time, p + 3, 5);

	if(pTDT->table_id == 0x73)
	{
		nLen = pTDT->section_length + 3;
		nDesLen = nLen - 8 - 4;
		memcpy(pTDT->descriptor, p + 10, nDesLen);
		pTDT->descriptor[nDesLen] = '\0';
	}

}

/*********************************************************************************************************************************
* Function Name : GetTheCurentTime
* Description : out put the start time
* Parameters :
* pucTime -- the buffer of time
* Returns : void
*********************************************************************************************************************************/
static void GetTheCurentTime(unsigned char *pucTime, char *aucStartTime)
{

	int iDay = 0;
	int iTemp = 0;
	int iYear = 0;
	int iMonth = 0;
	unsigned int uiMJD = 0;
	unsigned char second = 0;
	unsigned char min = 0;
	unsigned char hour = 0;
	
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
		second = pucTime[4];
		min    = pucTime[3];
		hour   = pucTime[2];
		if(second >= 60)
		{ 
			second -= 60;
			min ++;
		}
		if(min >= 60)
		{ 
			min -= 60;
			hour++;
		}		
		if(hour >= 24)
		{
			hour -= 24;
		}		
		
		sprintf(aucStartTime,"%d/%d/%d %02d:%02d:%02d",iYear,iMonth,iDay,hour,min,second);
		//logd("the system current time is %s\n",aucStartTime);

	}
}

/*********************************************************************************************************************************
* Function Name : GetTdtInformation
* Description : Get the TDT information
* Parameters :
* pTDT -- the TS_TDT
* pProgram -- the PROGRAM
* Returns : void
*********************************************************************************************************************************/
static void GetTdtInformation(TS_TDT *pTDT, char *pCurTime)
{

	unsigned int nDesLen = 0;
	LOCAL_TIME_OFFSET_DESCRIPTOR Descriptor = {0}; // for TDT table
	GetTheCurentTime(pTDT->UTC_time, pCurTime);
	nDesLen = pTDT->section_length - 8 - 4;
	
	if (0 == GetLocalTimeOffsetDescriptor(&Descriptor, pTDT->descriptor, nDesLen))
	{
		printf("GetLocalTimeOffsetDescriptor \n");
	}

}

/*********************************************************************************************************************************
* Function Name : ParseFromTDT
* Description : parse the TDT table and put the information in program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromTDT(unsigned char *p, char *pCurTime)
{

	TS_TDT mTsTDT = {0};
	ParseTDT(&mTsTDT, p); //parse the TDT	
	GetTdtInformation(&mTsTDT, pCurTime);
	return 0;

}
