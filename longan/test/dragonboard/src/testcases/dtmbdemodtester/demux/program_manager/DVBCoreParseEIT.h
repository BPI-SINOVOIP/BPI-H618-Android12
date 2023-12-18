/********************************************************************************************************************************
* All Rights GLOBALSAT Reserved *
********************************************************************************************************************************
********************************************************************************************************************************
* Filename : DVBCoreParseEIT.h
* Description : parse the EIT
* Version : 1.0
* History :
* xhw : 2015-9-22 Create
*********************************************************************************************************************************/

#ifndef _PARSE_EIT_H_
#define _PARSE_EIT_H_
#define DESCRIPTOR_MAX_LENGTH_4096 4096

typedef struct TS_EIT_EVENT
{	
	unsigned int service_id :16;
	unsigned int event_id :16;
	unsigned char start_time[5];
	unsigned int duration :24;	
	unsigned int running_status :3;
	unsigned int free_CA_mode :1;
	unsigned int descriptors_loop_length :12;
	unsigned char descriptor[DESCRIPTOR_MAX_LENGTH_4096];
	struct TS_EIT_EVENT *next;

} TS_EIT_EVENT;

typedef struct TS_EIT
{

	unsigned int table_id :8;
	unsigned int section_syntax_indicator :1;
	unsigned int reserved_future_use_1 :1;
	unsigned int reserved_1 :2;
	unsigned int section_length :12;
	unsigned int service_id :16;
	unsigned int reserved_2 :2;
	unsigned int version_number :5;
	unsigned int current_next_indicator :1;
	unsigned int section_number :8;
	unsigned int last_section_number :8;
	unsigned int transport_stream_id :16;
	unsigned int original_network_id :16;
	unsigned int segment_last_section_number :8;
	unsigned int last_table_id :8;
	TS_EIT_EVENT *pEitEventHead;
	unsigned int CRC_32 :32;

} TS_EIT;

#if 0
typedef struct _TS_EIT_SERVICE_INFO
{
	unsigned int	serviceId;
	unsigned char	pfCurSectionMask;
	unsigned char	pfVersionNumber;

	TS_EIT_EVENT*	pfEventHeader;
	
	TS_EIT_EVENT	*pf_info;
	
	TS_EIT_EVENT	*sch_info;
	
	TS_EIT_EVENT	*current_info;
	
	TS_EIT_EVENT	*next_info;
	
	unsigned int	schdlCurSectionNum[EIT_SCHEDULE_TABLE_ID_NUM];
	unsigned char	schdlVersionNumber[EIT_SCHEDULE_TABLE_ID_NUM];
	unsigned char	schdlSectionMask[EIT_SCHEDULE_TABLE_ID_NUM][32];

	TS_EIT_EVENT*	schdlEventHeader[EIT_SCHEDULE_TABLE_ID_NUM];
	struct _TS_EIT_SERVICE_INFO* next;

}TS_EIT_SERVICE_INFO;
#endif


/*********************************************************************************************************************************
* Function Name : ParseFromEIT
* Description : parse the EIT table and print the current program information
* Parameters :
* p -- the buffer of section
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ParseFromEIT(unsigned char *p, PROGRAM_EVENT **pProgramEvent, unsigned char* pVersionNumber, SCH_SEC_MASK* pMask, unsigned int pro_num, unsigned int *event_cnt);

#endif /* PARSEEIT_H_ */
