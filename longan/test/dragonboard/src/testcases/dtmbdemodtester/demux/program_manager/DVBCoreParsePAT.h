/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParsePAT.h
* Description : parse the PAT
* Version : 2.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/

#ifndef _PARSE_PAT_H_
#define _PARSE_PAT_H_
typedef struct TS_PAT_PROGRAM
{

	unsigned int program_number :16;
	unsigned int reserved_3 :3;
	unsigned int program_map_PID :13;
	struct TS_PAT_PROGRAM *next;

} TS_PAT_PROGRAM;

typedef struct TS_PAT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int zero :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned int transport_stream_id :16;
	unsigned int reserved_2 :2;
	unsigned int version_number :5;
	unsigned int current_next_indicator :1;
	unsigned int section_number :8;
	unsigned int last_section_number :8;

	TS_PAT_PROGRAM *pPatProgramHead;
	unsigned int network_PID :13;

	unsigned CRC_32 :32;

} TS_PAT;

/*********************************************************************************************************************************
* Function Name : ParseFromPAT
* Description : parse the PAT table and put the information in PROGRAM link list
* Parameters :
* p -- the buffer of section
* pProgram -- the program where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/

int ParseFromPAT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber);
#endif /* PARSEPAT_H_ */
