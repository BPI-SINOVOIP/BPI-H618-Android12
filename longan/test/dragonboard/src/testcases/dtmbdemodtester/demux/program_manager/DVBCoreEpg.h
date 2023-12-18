#ifndef _DVBCORE_TS_PARSER_H_
#define _DVBCORE_TS_PARSER_H_

#include <DVBCoreTypes.h>
#include <DVBCoreList.h>
#include <DVBCoreBuffer.h>
#include <DVBCoreAtomic.h>
#include "DVBCoreProgram.h"
#include "DVBCoreProgramEvent.h"

#define NB_PID_MAX 8192
#define MAX_SECTION_SIZE 4096
//* codec id

#define CODEC_ID_DVB_SUBTITLE       18
#define CODEC_ID_BLUERAY_SUBTITLE   19

//* packet size of different format
#define TS_FEC_PACKET_SIZE          204
#define TS_DVHS_PACKET_SIZE         192
#define TS_PACKET_SIZE              188

/* pids */
#define PAT_PID                 0x0000
#define SDT_BAT_PID             0x0011
#define CAT_PID					0x0001
#define TSDT_PID				0x0002
#define NIT_PID					0x0010
#define EIT_PID					0x0012
//#define RST_PID					0x0012
#define TDT_TOT_PID				0x0014
#define DIT_PID					0x001e
#define SIT_PID					0x001f
#define PID_NULL				0x1fff

/* table ids */
#define PAT_TID                 0x00
#define PMT_TID                	0x02
#define SDT_TID                 0x42
#define M4OD_TID                0x05

#define PAT_BUF_IDX   0
#define PMT_BUF_IDX   0
#define SDT_BUF_IDX   0
#define EIT_BUF_IDX   0
#define CAT_BUF_IDX   0
#define TDT_BUF_IDX   1
#define NIT_BUF_IDX   0
#define SECTION_BUF_SIZE   	(4096*188)
#define SECTION_BUFIDX_CNT   8 

typedef enum
{
	EPG_TYPE_NONE = 0,
	EPG_TYPE_PF   = 0X01,
	EPG_TYPE_SCH  = 0X02,
	EPG_TYPE_BOTH = 0X03,	//EPG_TYPE_PF | EPG_TYPE_SCH
}EPG_TYPE_E;

typedef enum
{
	SUCCESS = 0,
	FAIL = -1,
	SKIP = -2,
}status_t;


typedef enum
{
	TYPE_UNKNOWN	= -1,
    TYPE_VIDEO      = 0,
    TYPE_AUDIO		= 1,
    TYPE_SUBS		= 2,
}MediaType;

typedef struct
{
    struct DVBCoreListNodeS node;
	unsigned PID;
	dvbcore_bool mPayloadStarted;
	DVBCoreBufferT *mBuffer;
}PSISection;


typedef struct TSParserS TSParser;

struct TSParserS
{
    struct DVBCoreListS mPSISections;
	PROGRAM *pProgram;
	//PROGRAM_EVENT *pProgramEvent; /* schedules sort by time */
	PROGRAM_EVENT *pProgramEvent[EIT_SCHEDULE_TABLE_ID_NUM];/*schedule 8 days at most*/
	//unsigned int	schdlSecNum[EIT_SCHEDULE_TABLE_ID_NUM];
	unsigned char	schdl_eit_version_number[EIT_SCHEDULE_TABLE_ID_NUM];
	//unsigned char	schdl_sec_mask[EIT_SCHEDULE_TABLE_ID_NUM][32];
	SCH_SEC_MASK	schdl_sec_mask[EIT_SCHEDULE_TABLE_ID_NUM][32];
	//unsigned char	pf_sec_mask;
	SCH_SEC_MASK	pf_sec_mask;
	unsigned int    prog_num;
	uint8           schdl_count[EIT_SCHEDULE_TABLE_ID_NUM];
	uint8           schdl_parse_end[EIT_SCHEDULE_TABLE_ID_NUM];
	uint8			pf_count;
    uint8           pf_parse_end;
    unsigned int    event_total_count;//count may be more than 255
	PROGRAM_EVENT *pPresentEvent; /* present program */
	PROGRAM_EVENT *pFollowEvent; /* following program */
	
	char CurTime[25];
	EPG_TYPE_E type;
	uint8 mProgramCount;
    unsigned char pat_version_number;
	unsigned char pmt_version_number;
	unsigned char sdt_version_number;
	unsigned char pf_eit_version_number;
	unsigned char nit_version_number;
	unsigned char cat_version_number;
	
	int	bIsEndSdtSection;
	int	bIsEndPatSection;
	int	bIsEndPmtSection;
	int	bIsEndNitSection;
	int	bIsEndEitSection;
	int	bIsEndCatSection;

	unsigned char*	MallocBufer;
	unsigned char*	SectionBufer[SECTION_BUFIDX_CNT];
	unsigned char 	bIsBufferFull[SECTION_BUFIDX_CNT];
	unsigned char enablePid[NB_PID_MAX];/*0: discard*/
    pthread_mutex_t mutex;
    pthread_cond_t cond;
	uint32 attribute;
	uint32 forceStop;
    int mErrno;
};


// interface
status_t feedTSPacket(TSParser *mTSParser, const uint8 *data, int size);
status_t parseSectionPacket(TSParser *mTSParser, int mSectionPid,const uint8 *data, int size);

TSParser * TSParserOpen(void);
void TSParserInit(TSParser *mTSParser);
int TSParserClose(TSParser *mTSParser);
#endif

