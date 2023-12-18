/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParsePMT.h
* Description : parse the PAT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/

#ifndef _PARSE_PMT_H_
#define _PARSE_PMT_H_
#define DESCRIPTOR_MAX_LENGTH_4096 4096

typedef struct TS_PMT_STREAM
{

	unsigned int stream_type :8;
	unsigned int reserved_5 :3;
	unsigned int elementary_PID :13;
	unsigned int reserved_6 :4;
	unsigned int ES_info_length :12;
	unsigned char descriptor[DESCRIPTOR_MAX_LENGTH_4096];
	struct TS_PMT_STREAM *next;

} TS_PMT_STREAM;

typedef struct TS_PMT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int zero :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned int program_number :16;
	unsigned int reserved_2 :2;
	unsigned int version_number :5;
	unsigned int current_next_indicator :1;
	unsigned int section_number :8;
	unsigned int last_section_number :8;
	unsigned int reserved_3 :3;
	unsigned int PCR_PID :13;
	unsigned int reserved_4 :4;
	unsigned int program_info_length :12;
	unsigned char program_info_descriptor[DESCRIPTOR_MAX_LENGTH_4096];

	TS_PMT_STREAM *pPmtStreamHead;

	unsigned int CRC_32 :32;

} TS_PMT;

/*********************************************************************************************************************************
* Function Name : ParseFromPMT
* Description : parse the PMT table and put the information in program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromPMT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber);

#endif /* PARSEPMT_H_ */