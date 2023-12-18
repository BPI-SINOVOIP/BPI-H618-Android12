/****************************************************************************
* All Rights GLOBALSAT Reserved *
*****************************************************************************
*****************************************************************************
* Filename : DVBCoreProgram.h
* Description: some actions for program information
* Version: 1.0
* History:
* xhw : 2015-9-22 Create
*****************************************************************************/
#ifndef _DVBCORE_PROGRAM_H_
#define _DVBCORE_PROGRAM_H_

#include <dc_types.h>

#define MAX_AUDIO_NUM 32

/* descriptor ids */
#define DVB_SUBT_DESCID             0x59

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_AUDIO_MPEG4     0x11
#define STREAM_TYPE_OMX_VIDEO_CodingAVC      0x1b
#define STREAM_TYPE_VIDEO_H265      0x24
#define STREAM_TYPE_VIDEO_VC1       0xea

#define STREAM_TYPE_PCM_BLURAY      0x80    //* add for blue ray
#define STREAM_TYPE_DVBCORE_AUDIO_AC3   0x81
#define STREAM_TYPE_AUDIO_HDMV_DTS  0x82
#define STREAM_TYPE_DVBCORE_AUDIO_AC3_TRUEHD    0x83
#define STREAM_TYPE_AUDIO_EAC3      0x84
#define STREAM_TYPE_DVBCORE_AUDIO_DTS_HRA   0x85    //* add for blue ray
#define STREAM_TYPE_DVBCORE_AUDIO_DTS_MA    0x86    //* add for blue ray
#define STREAM_TYPE_DVBCORE_AUDIO_DTS       0x8a
#define STREAM_TYPE_DVBCORE_AUDIO_AC3_      0xa1
#define STREAM_TYPE_DVBCORE_AUDIO_DTS_      0xa2
#define STREAM_TYPE_DVBCORE_VIDEO_MVC		0x20
#define STREAM_TYPE_HDMV_PGS_SUBTITLE 0x90
//#define STREAM_TYPE_SUBTITLE_DVB    0x100
#define STREAM_TYPE_VIDEO_AVS		0x42

typedef struct _PROGRAM
{
	unsigned int uProgramNumber :16;
	unsigned int uPMT_PID :13;
	unsigned int uPCR_PID :13;
	unsigned int uVideoPID :13;
	unsigned int uAudioPID[MAX_AUDIO_NUM];
	unsigned int EIT_schedule_flag :1;
	unsigned int EIT_present_following_flag :1;
	char pucProgramName[DC_NAME_LENGTH];
	char aucStartTime[25];
	unsigned char nFreeCAMode :1; //0: free; 1: ca.
	unsigned int CA_system_ID :16;
	unsigned int ECM_PID :13;
	unsigned int EMM_PID :13;
	struct _PROGRAM *next;

} PROGRAM;

/*********************************************************************************************************************************
* Function Name : InitialProgram
* Description: initial program
* Parameters:
* pProgram -- the program which we should free memory
* Returns: void
*********************************************************************************************************************************/
void InitialProgram(PROGRAM *pProgram);

/*********************************************************************************************************************************
* Function Name : PutInProgramLinkList
* Description: put the program to link list
* Parameters:
* pProgramHead -- the link list header
* pProgram -- the program we should put
* Returns: void
*********************************************************************************************************************************/
void PutInProgramLinkList(PROGRAM **pProgramHead, PROGRAM *pProgram);

/*********************************************************************************************************************************
* Function Name : PintProgramInformation
* Description: out put all of the program information
* Parameters:
* pProgram -- the program
* Returns: void
*********************************************************************************************************************************/
void PintProgramInformation(PROGRAM *pProgram);

/*********************************************************************************************************************************
* Function Name : FreeProgramMemory
* Description: free the memory of program
* Parameters:
* pProgram -- the program which we should free memory
* Returns: void
*********************************************************************************************************************************/
void FreeProgramMemory(PROGRAM *pProgram);

#endif
