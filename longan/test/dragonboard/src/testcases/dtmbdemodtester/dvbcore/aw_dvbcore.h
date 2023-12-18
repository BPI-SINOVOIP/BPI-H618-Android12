
#ifndef AW_DVB_CORE_H
#define AW_DVB_CORE_H

#include <stdint.h>
#include "dc_demod_common.h"

#define STR_LENGTH 512
#define MAX_AUDIO_NUM 32
#define MAX_EPG_SEARCH_NUM 32
#define MAX_LOST_SIGNAL_CNT 3

typedef struct DVBCoreS DVBCoreT;

typedef enum DvbControlCmdS
{
    UNKNOWN_CONTROL_CMD = 0,
    STOP_SEARCH_PROGRAM_CMD,
    STOP_SEARCH_EPG_CMD,

}DvbControlCmdT;

typedef enum DvbNotifyMsgS
{
    DVB_UNKNOWED_MSG,
    DVB_LOST_SIGNAL ,
    DVB_RECOVER_SIGNAL
}DvbNotifyMsgT;

typedef enum DvbStateS
{
    DVB_STATE_UNINITED          = 1 << 0,
    DVB_STATE_INITED            = 1 << 1,
    DVB_STATE_SEARCHING_PROGRAM = 1 << 2,
    DVB_STATE_SEARCHING_EPG     = 1 << 3,
    DVB_STATE_STARTD            = 1 << 4,
    DVB_STATE_STOPED            = 1 << 5,
}DvbStateT;

/* signal info */
typedef struct SignalInfoS
{
    int quality;
    int strength;
    /** fixme,other info of tuner need to be included **/
}SignalInfoT;

/* program info */
#define PROGRAM_FLAGS_SCRAMBLE 0x01
#define PROGRAM_INVALID_PID 0x1FFF

typedef struct ProgramIDS
{
    uint32_t freq;
    struct SignalInfoS signal;
    uint32_t scrambled;
    uint32_t free_CA_mode;
    uint32_t running_status;
    uint32_t ecm_pid;
    uint32_t pmt_pid;
    uint32_t pcr_pid;
    uint32_t video_pid;
    uint32_t audio_pid[MAX_AUDIO_NUM];
    uint32_t ProgramNumber;
}ProgramIDT;

typedef struct ProgramInfo
{
    ProgramIDT id;
    char ProgramName[STR_LENGTH];
    uint32_t ValidProgramCount;
    uint32_t EIT_schedule_flag;
    uint32_t EIT_present_following_flag;
    struct ProgramInfo *next;
}ProgramInfolistT;

/* EPG search params */
typedef struct EPGSearchParamS
{
    uint32_t SearchFreq;
    uint32_t SearchCount;
    uint32_t ServiceID[MAX_EPG_SEARCH_NUM];
}EPGSearchParamT;

/* EPG info */
typedef struct EPGInfoS
{
    uint32_t event_id;
    uint32_t running_status; //0: undefined;1: not running;2: starts in a few seconds;3: pausing;4: running.
    char start_time[32];
    char end_time[32];
    char duration[32];
    char event_name[STR_LENGTH];
    char text_char[STR_LENGTH];
    struct EPGInfoS *next;
}EPGInfoT;

typedef struct EpgEventInfolistS
{
    uint32_t search_freq;
    uint32_t service_id;
    uint32_t event_count;
    EPGInfoT *pInfos;
}EpgEventInfolist;

enum
{
    DC_STATUS_ERR = -1,
    DC_STATUS_OK = 0
};

/* callback function... */
typedef struct DCListenerS
{
    /* status, err status */
    int (*onTryFreqFinish)(void *cookie, int status, void *param);
    int (*onStartFinish)(void *cookie, int status, void *param);
    int (*onEPGSearch)(void *cookie, int status, void *param);
    int (*onNotify)(void *cookie, int messageId, void *param);
}DCListenerT;

#ifdef __cplusplus
extern "C" {
#endif

/* ������� */
DVBCoreT *DVBCoreCreate(DCListenerT *listener, DvbStandardType nDvbStandardType);

int DVBCoreDestroy(DVBCoreT *dc);

/*
*  ��ʼ�� TSC/Demod/Tunner/CA �ȡ�
*  �첽���ã����֮��ͨ���ص�listener��onPrepareFinish֪ͨ�û�
*/
int DVBCoreInit(DVBCoreT *dc, void *param);

int DVBCoreDeInit(DVBCoreT *dc);

/*
*  ������Ƶ���첽���ã����֮��ͨ��onTryFreqFinish֪ͨ�û������ҰѸ�Ƶ���Ŀ��Ϣ�ش����û���
*/
int DVBCoreTryFreqAsync(DVBCoreT *dc, TunerConnectParam mConnectParam);

/* ��ʼ�����demux�Ȳ��� */
int DVBCoreStartAsync(DVBCoreT *dc, TunerConnectParam mConnectParam, ProgramIDT mProgramID);


/* ��ȡEPG��Ϣ */
int DVBCoreEPGSearchAsync(DVBCoreT *dc, EPGSearchParamT mSearchParam);


int DVBCoreStop(DVBCoreT *dc);

/* ��ȡ��ǰ����Ƶ���ź���������δ��Ƶ����Ӧ��ֵΪ0 */
int DVBCoreGetSignalInfo(DVBCoreT *dc, SignalInfoT *si);

/*
 * ��������
 * 1. ����ѡ���
 * 2.
 * �ȵȵȣ�����չ��
 */

int DVBCoreControl(DVBCoreT *dc, DvbControlCmdT cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif

