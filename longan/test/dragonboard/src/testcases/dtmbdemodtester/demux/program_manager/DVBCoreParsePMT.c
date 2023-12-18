/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParsePMT.c
* Description : parse the PAT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVBCoreProgram.h"
#include "DVBCoreDescriptor.h"
#include "DVBCoreParsePMT.h"
#include "DVBCoreMemory.h"

/*********************************************************************************************************************************
* Function Name : InitialTsPmtStream
* Description : initial the TS_PMT_STREAM
* Parameters :
* pPmtStream -- the TS_PMT_STREAM
* Returns : void
*********************************************************************************************************************************/
static void InitialTsPmtStream(TS_PMT_STREAM *pPmtStream)
{

	pPmtStream->stream_type = 0x00;
	pPmtStream->reserved_5 = 0x00;
	pPmtStream->elementary_PID = 0x0000;
	pPmtStream->reserved_6 = 0x00;
	pPmtStream->ES_info_length = 0x0000;
	memset(pPmtStream->descriptor, '\0', DESCRIPTOR_MAX_LENGTH_4096);
	pPmtStream->next = NULL;

}

/*********************************************************************************************************************************
* Function Name : PutInTsPmtStreamLinkList
* Description : put the TS_PMT_STREAM to link list
* Parameters :
* pPmtStreamHead -- the link list header
* pPmtStream -- the TS_PMT_STREAM which we should put in the link list
* Returns : void
*********************************************************************************************************************************/
static void PutInTsPmtStreamLinkList(TS_PMT_STREAM **pPmtStreamHead, TS_PMT_STREAM *pPmtStream)
{

	TS_PMT_STREAM *pPmtStreamTemp = *pPmtStreamHead;

	if (NULL == (*pPmtStreamHead))
	{
		(*pPmtStreamHead) = pPmtStream;
	}
	else
	{
		while (NULL != pPmtStreamTemp->next)
		{
			pPmtStreamTemp = pPmtStreamTemp->next;
		}
		pPmtStreamTemp->next = pPmtStream;
	}

}

/*********************************************************************************************************************************
* Function Name : FreeTsPmtStreamLinkList
* Description : free the stream link list
* Parameters :
* pPmtStream -- the stream first address
* Returns : void
*********************************************************************************************************************************/
static void FreeTsPmtStreamLinkList(TS_PMT_STREAM **pPmtStream)
{

	TS_PMT_STREAM *pPmtStreamTemp = NULL;

	while (NULL != *pPmtStream)
	{
		pPmtStreamTemp = (*pPmtStream)->next;
		DVBCoreFree(*pPmtStream);
		*pPmtStream = pPmtStreamTemp;
	}
	*pPmtStream = NULL;

}

/*********************************************************************************************************************************
* Function Name : ParsePMT
* Description : parse the pat table
* Parameters :
* pPMT -- the PMT
* p -- the buffer of PMT section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
static int ParsePMT(TS_PMT *pPMT, unsigned char *p, unsigned char* pVersionNumber)
{

	int nLen = 0;
	int nPos = 12;
	TS_PMT_STREAM * pPmtStreamMalloc = NULL;

	pPMT->pPmtStreamHead = NULL;
	pPMT->table_id = p[0];
	pPMT->section_syntax_indicator = p[1] >> 7;
	pPMT->zero = (p[1] >> 6) & 0x01;
	pPMT->reserved_1 = (p[1] >> 4) & 0x03;
	pPMT->section_length = ((p[1] & 0x0F) << 8) | p[2];
	pPMT->program_number = (p[3] << 8) | p[4];
	pPMT->reserved_2 = p[5] >> 6;
	pPMT->version_number = (p[5] >> 1) & 0x1F;
	pPMT->current_next_indicator = (p[5] << 7) >> 7;
    if(pPMT->version_number == *pVersionNumber || !pPMT->current_next_indicator)
    {
		return 0;
    }
	pPMT->section_number = p[6];
	pPMT->last_section_number = p[7];
	pPMT->reserved_3 = p[8] >> 5;
	pPMT->PCR_PID = ((p[8] << 8) | p[9]) & 0x1FFF;
	pPMT->reserved_4 = p[10] >> 4;
	pPMT->program_info_length = ((p[10] & 0x0F) << 8) | p[11];
	if (0 != pPMT->program_info_length)
	{
		memcpy(pPMT->program_info_descriptor, p + 12, pPMT->program_info_length);
		pPMT->program_info_descriptor[pPMT->program_info_length] = '\0';
	}
	nLen = pPMT->section_length + 3;
	pPMT->CRC_32 = (p[nLen - 4] << 24) | (p[nLen - 3] << 16) | (p[nLen - 2] << 8) | p[nLen - 1];
	
	/*----------------------------------------------------------*/
	/* get the TS_PMT_STREAM and put in a linked list */
	/*----------------------------------------------------------*/
	for (nPos += pPMT->program_info_length; nPos < nLen - 4; nPos += 5)
	{
		pPmtStreamMalloc = (TS_PMT_STREAM *)DVBCoreMalloc(sizeof(TS_PMT_STREAM));
		if (NULL == pPmtStreamMalloc)
		{
			printf("PMT stream DVBCoreMalloc failed.");
			return -1;

		}
		InitialTsPmtStream(pPmtStreamMalloc);
		pPmtStreamMalloc->stream_type = p[nPos];
		pPmtStreamMalloc->reserved_5 = p[nPos + 1] >> 5;
		pPmtStreamMalloc->elementary_PID = ((p[nPos + 1] << 8) | p[nPos + 2]) & 0x1FFF;
		pPmtStreamMalloc->reserved_6 = p[nPos + 3] >> 4;
		pPmtStreamMalloc->ES_info_length = ((p[nPos + 3] & 0x0F) << 8) | p[nPos + 4];
		if (pPmtStreamMalloc->ES_info_length != 0)
		{
			memcpy(pPmtStreamMalloc->descriptor, p + 5 + nPos, pPmtStreamMalloc->ES_info_length);
			pPmtStreamMalloc->descriptor[pPmtStreamMalloc->ES_info_length] = '\0';
			nPos += pPmtStreamMalloc->ES_info_length;
		}
		PutInTsPmtStreamLinkList(&(pPMT->pPmtStreamHead), pPmtStreamMalloc);

	}
	return 0;

}

/*********************************************************************************************************************************
* Function Name : PutStreamInformationToProgram
* Description : put the information(audio PID video PID) we get from PMT in the program
* Parameters :
* pInProgram -- the program (we put information in it)
* pStream -- the PMT stream
* Returns : void
*********************************************************************************************************************************/
static void PutStreamInformationToProgram(PROGRAM *pInProgram, TS_PMT_STREAM *pStream)
{

	int iFlag = 0;
	TS_PMT_STREAM *pTemp = pStream;
	int i = 0;
	while (NULL != pTemp)
	{
		switch(pTemp->stream_type)
		{
		#if 0
		case STREAM_TYPE_AUDIO_MPEG1:
		case STREAM_TYPE_AUDIO_MPEG2:
		case STREAM_TYPE_AUDIO_MPEG4:
		case STREAM_TYPE_DVBCORE_AUDIO_AC3:
		case STREAM_TYPE_DVBCORE_AUDIO_AC3_:
		case STREAM_TYPE_AUDIO_EAC3:
		case STREAM_TYPE_DVBCORE_AUDIO_AC3_TRUEHD:
		case STREAM_TYPE_DVBCORE_AUDIO_DTS:
		case STREAM_TYPE_DVBCORE_AUDIO_DTS_:
		case STREAM_TYPE_AUDIO_HDMV_DTS:
		case STREAM_TYPE_DVBCORE_AUDIO_DTS_HRA:
		case STREAM_TYPE_DVBCORE_AUDIO_DTS_MA:
		case STREAM_TYPE_AUDIO_AAC:
			{
				pInProgram->uAudioPID[i] = pTemp->elementary_PID;
				i ++;
				break;
			}
		#endif
		case STREAM_TYPE_VIDEO_MPEG1:
		case STREAM_TYPE_VIDEO_MPEG2:
		case STREAM_TYPE_VIDEO_MPEG4:
		case STREAM_TYPE_VIDEO_AVS:
		case STREAM_TYPE_OMX_VIDEO_CodingAVC:
		case STREAM_TYPE_DVBCORE_VIDEO_MVC:
		case STREAM_TYPE_VIDEO_H265:
		case STREAM_TYPE_VIDEO_VC1:
		//case STREAM_TYPE_SUBTITLE_DVB:
		case STREAM_TYPE_PCM_BLURAY:
		case STREAM_TYPE_HDMV_PGS_SUBTITLE:
			pInProgram->uVideoPID = pTemp->elementary_PID;
			break;
		#if 0
		case STREAM_TYPE_PRIVATE_DATA:
			{
				if(pTemp->descriptor[0] == 0x52) // fixed me
				{
					pInProgram->uAudioPID[i] = pTemp->elementary_PID;
					i ++;
				}
				break;
			}
		#endif

		default:
			{
				pInProgram->uAudioPID[i] = pTemp->elementary_PID;
				i ++;
				break;
			}
		}
		pTemp = pTemp->next;

	}

}

/*********************************************************************************************************************************
* Function Name : PutPmtInfomationToProgramList
* Description : free the memory of the TS_PAT_PROGRAM link list
* Parameters :
* pPMT -- the TS_PMT
* pProgram -- the program where we put the information
* Returns : void
*********************************************************************************************************************************/
static void PutPmtInfomationToProgramList(TS_PMT *pPMT, PROGRAM **pProgram)
{

	TS_PMT_STREAM *pPmtStream = NULL;
	PROGRAM *pProgramTemp = *pProgram;
	CA_IDENTIFIER_DESCRIPTOR Descriptor = {0}; // for CAT table get the ECM_PID

	pPmtStream = pPMT->pPmtStreamHead;
	while (NULL != pProgramTemp)
	{
		if (pProgramTemp->uProgramNumber == pPMT->program_number)
		{
			pProgramTemp->uPCR_PID = pPMT->PCR_PID;
			if (0 == GetCaIdentifierDescriptor(&Descriptor, pPMT->program_info_descriptor,pPMT->program_info_length))
			{
				pProgramTemp->ECM_PID = Descriptor.ca_pid; //get the ECM_PID in the PMT table
				printf("PMT-ECM_PID: %d", pProgramTemp->ECM_PID);

			}

			if (pProgramTemp->ECM_PID == 0)
			{
				struct TS_PMT_STREAM *pmt_stream = pPMT->pPmtStreamHead;
				while (pmt_stream)
				{
					if (0 == GetCaIdentifierDescriptor(&Descriptor, pmt_stream->descriptor, pmt_stream->ES_info_length))
					{
						pProgramTemp->ECM_PID = Descriptor.ca_pid; //get the ECM_PID in the PMT table
						printf("PMT-ECM_PID: %d", pProgramTemp->ECM_PID);
						break;
					}
					pmt_stream = pmt_stream->next;
				}
			}
			PutStreamInformationToProgram(pProgramTemp, pPmtStream);
		}
		pProgramTemp = pProgramTemp->next;

	}
	return ;
}

/*********************************************************************************************************************************
* Function Name : ParseFromPMT
* Description : parse the PMT table and put the information in program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromPMT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber)
{
	TS_PMT mTsPMT = {0};

	if (-1 == ParsePMT(&mTsPMT, p, pVersionNumber)) //parse the PMT
	{
		printf("ParsePMT error!\n");
		return -1;
	}
	PutPmtInfomationToProgramList(&mTsPMT, pProgram);
	FreeTsPmtStreamLinkList(&(mTsPMT.pPmtStreamHead));
	return 0;

}

