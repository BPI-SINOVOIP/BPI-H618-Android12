/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseSDT.h
* Description : parse the SDT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/

#ifndef _PARSE_SDT_H_
#define _PARSE_SDT_H_
#define DESCRIPTOR_MAX_LENGTH_4096 4096

typedef struct TS_SDT_SERVICE
{

	unsigned int service_id :16;
	unsigned int reserved_future_use :6;
	unsigned int EIT_schedule_flag :1;
	unsigned int EIT_present_following_flag :1; //0x0F
	unsigned int running_status :3;
	unsigned int free_CA_mode :1;
	unsigned int descriptors_loop_length :12;
	unsigned char descriptor[DESCRIPTOR_MAX_LENGTH_4096];
	struct TS_SDT_SERVICE *next;

} TS_SDT_SERVICE;

typedef struct TS_SDT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int reserved_future_use_1 :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned int transport_stream_id :16;
	unsigned int reserved_2 :2;
	unsigned int version_number :5;
	unsigned int current_next_indicator :1;
	unsigned int section_number :8;
	unsigned int last_section_number :8;
	unsigned int original_network_id :16;
	unsigned int reserved_future_use_2 :8;
	TS_SDT_SERVICE *pSdtServiceHead;
	unsigned int CRC_32 :32;

} TS_SDT;

/*********************************************************************************************************************************
* Function Name : ParseFromSDT
* Description : parse the SDT table and put the information in the program structure
* Parameters :
* p -- the buffer of section
* pProgram -- the program structure where we put the information
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromSDT(unsigned char *p, PROGRAM **pProgram, unsigned char* pVersionNumber);
#endif /* PARSESDT_H_ */
