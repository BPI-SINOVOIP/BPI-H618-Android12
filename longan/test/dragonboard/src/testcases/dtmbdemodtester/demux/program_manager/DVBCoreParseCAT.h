/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseCAT.h
* Description : parse the CAT
* Version : 1.0
* History :
* xhw : 2015-9-15 Create
*********************************************************************************************************************************/

#ifndef PARSECAT_H_
#define PARSECAT_H_
#define DESCRIPTOR_MAX_LENGTH_4096 4096

typedef struct TS_CAT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int zero :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned int reserved_2 :18;
	unsigned int version_number :5;
	unsigned int current_next_indicator :1;
	unsigned int section_number :8;
	unsigned int last_section_number :8;
	unsigned int transport_stream_id :16;
	unsigned char descriptor[DESCRIPTOR_MAX_LENGTH_4096];
	unsigned int CRC_32 :32;

} TS_CAT;

/*********************************************************************************************************************************
* Function Name : ParseFromCAT
* Description : parse the CAT table and put the information in program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromCAT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber);

#endif /* PARSECAT_H_ */
