/**@file
*    @brief        DVBCoreͷ�ļ�
*    @author        xiebin
*    @date        2015-10-8
*    @version        0.1
*    @note        AllWinnerTech. All rights reserved.
*/

#ifndef _AW_DVB_CORE_H_
#define _AW_DVB_CORE_H_
/****************************INCLUDE FILE************************************/
#include <stdint.h>
#include <dc_demod.h>

/****************************GLOBAL MACRO************************************/
#define AW_DVBCORE_SUCCESS 0
#define AW_DVBCORE_FAILURE -1
/** fixme ,define module error code based on the implement of the specific module **/


/****************************GLOBAL TYPE************************************/

typedef int (*PMCallback)(void* pUserData, int eMessageId, void* param);
typedef void (*EPGCallback)(int status, int event_count, void *param);
typedef void (*pfun_cas_event)(uint32_t event,uint32_t param1,uint32_t param2,uint32_t param3);

#define CALLBACK_EVENT_CA    0x01
#define CALLBACK_EVENT_TUNER 0X02


#define MAX_AUDIO_NUM 32
#define MAX_PID_NUM   8192

typedef struct
{
    /** fixme,reserved.add it if needed **/
}PMInitParam;

enum PMCmd
{
    DVBCORE_PM_CMD_GET_PROGRAM,
    DVBCORE_PM_CMD_SCAN_END,
    DVBCORE_PM_CMD_END
};

typedef struct _PMProgramInfo
{
    /** fixme,if other info is needed,just add it here,set it to 0x1fff if the specific pid is not found **/
    short ProgramNumber;
    char ProgramName[512];
    short ecm_pid;
    short pmt_pid;
    short pcr_pid;
    short video_pid;
    short audio_pid[MAX_AUDIO_NUM];

    short EIT_schedule_flag;
    short EIT_present_following_flag;
    struct _PMProgramInfo *next;
}PMProgramInfolist;



typedef struct
{
    unsigned int    year;
    unsigned int    month;
    unsigned int    day;
    unsigned int    weekDay;
    unsigned char    hour;
    unsigned char    min;
    unsigned char    sec;

} TIME;


typedef struct
{
    /** fixme,reserved.add it if needed **/
    EPGCallback  pEpgCb;
}EPGInitParam;

typedef struct _EPGInfo
{
    /** fixme,defined later **/
    unsigned int service_id;
    unsigned int event_id;
    char start_time[25];
    char duration[25];
    unsigned int running_status; //0: undefined;1: not running;2: starts in a few seconds;3: pausing;4: running.
    unsigned int free_CA_mode;
    unsigned char event_name[512];
    unsigned char text_char[512];
    struct _EPGInfo *next;
}EPGInfolist;

typedef int SPStreamHandle;
typedef struct
{
    /**fixme,add param if needed **/
}SPInitParam;

typedef struct
{
    /** fixme:add other param if needed **/
    short pid_video;
    short pid_audio[MAX_AUDIO_NUM];
    short pid_pcr;
    short pid_pmt;
    int freq;
    short programNumber;
}SPStreamParam;

/****************************STATIC TYPE************************************/
#if defined(TULIP_TFLX)
///////////////////////////////////////////////////////////////////////////
/*CA��Ϣ*/
#define CA_MAX_NAME_LENGTH                             20
#define CA_MJD_UTC_CODING_LENGTH                           5
#define MAX_PRODUCT_RECORD_COUNT                    500
#define MAX_PACKAGE_RECORD_COUNT                    10
#define SN_LENGTH                                   16
// �û��鿴��Ȩʹ�õ����ݽṹ
struct info_product_
{
    unsigned short    m_id;
    unsigned char     m_type;
    unsigned char      m_nameLength;
    unsigned char      m_name[CA_MAX_NAME_LENGTH];
    unsigned int      m_expired[CA_MJD_UTC_CODING_LENGTH];           // ��ƷʧЧ���ڣ�MJD
};
typedef struct info_product_ Tfca_product_t;

struct info_user_entitlement_
{
    unsigned short      m_count;
    Tfca_product_t  m_entitlements[MAX_PRODUCT_RECORD_COUNT + MAX_PACKAGE_RECORD_COUNT];
};
typedef struct info_user_entitlement_    Tfca_user_entitlement_t;

struct info_HID_UserID_
{
    unsigned char    m_HIDlen;
    unsigned char    m_HID[40];
    unsigned char    m_UserIDlen;
    unsigned char    m_UserID[16];
    unsigned char    m_CalibVersionLen;
    unsigned char    m_CalibVersion[100];
    unsigned char    m_CaSNLen;
    unsigned char    m_CaSN[32];
};
typedef struct info_HID_UserID_ stHID_UserID_info;

// �û��鿴���ܿ�������Ϣʹ�õ����ݽṹ
struct info_user_privacy_
{
    unsigned char     m_version;                                  // ϵͳ�汾(CAϵͳ�汾)
    unsigned char     m_serialNumber[SN_LENGTH];                  // ���ܿ�����
    unsigned char     m_algorithmFamily;                          // ���ܿ���֧�ֵ��㷨�汾Ⱥ
    unsigned int    m_superCASID;                               // ��ǰCAϵͳ��
    unsigned char     m_pinAvailabilityFlag;                      // ������Ʊ�־
    unsigned char     m_defaultPINFlag;                           // Ĭ��PIN���ʶλ:1ΪĬ��PIN��Ĭ��ֵ����0Ϊ���Զ��壩
    unsigned char     m_systemApprovalCertificate;                // ϵͳ��ɳ��˼���
    unsigned char     m_userState;                                // �û�״̬��1���ã���0���ã�
    unsigned char     m_gcaNameLength;
    unsigned char     m_gcaName[CA_MAX_NAME_LENGTH];
    unsigned char     m_casProviderNameLength;
    unsigned char     m_casProviderName[CA_MAX_NAME_LENGTH];

};

typedef struct info_user_privacy_ stUser_privacy;

struct _Tfca_user_privacy_
{
    stUser_privacy m_stUser_privacy;
    stHID_UserID_info m_stHID_UserID_info;

};

typedef struct _Tfca_user_privacy_    Tfca_user_privacy_t;

///////////////////////////////////////////////////////////////////////////////////////
#endif

/****************************GLOBAL FUNC DECLEAR*****************************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 *   @brief           ��ʼ���ض�tuner ���� ���õ���tuner �Ĳ������
 *   @param[in]    param:��Ҫ��ʼ��tuner �Ĳ���
 *   @param[out]   handle:ָ��ǰ��ʼ����tuner ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreTunerInit(TunerCallback callback, TunerInitParam tunerInitParam);

int DVBCoreTunerConnectParamChange(TunerConnectParam mNewConnectParam);

/**
 *   @brief           �����ض�tuner �����Ӳ���
 *   @param[in]    handle:��ǰ��Ҫ�������Ӳ�����tuner ���
 *   @param[in]   connectParam:���Ӳ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *                  failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreTunerSetConnectParam(TunerHandle handle,TunerConnectParam *connectParam);

/**
 *   @brief           ���������ض�tuner
 *   @param[in]    handle:��ǰ��Ҫ �������ӵ�tuner ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note       �˺���Ϊ�첽���������Ϸ��ء���Ҫ����DVBCoreTunerGetLockedȥ��ѯ
 *               �ض�tuner������״̬
 *
 */
int DVBCoreTunerConnectAsync();

/**
 *   @brief           �Ͽ����ض�tuner������
 *   @param[in]    handle:��ǰ��Ҫ �Ͽ����ӵ�tuner ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note      �˺���Ϊͬ������
 *
 */
int DVBCoreTunerDisconnect(TunerHandle handle);


/**
 *   @brief           ��ȡ�ض�tuner������״̬
 *   @param[in]    handle:��ǰ��Ҫ ��ȡ����״̬��tuner ���
 *   @return        1:���ӳɹ�
 *                0 or others:δ������
 *   @note
 */
int DVBCoreTunerGetLocked(TunerHandle handle);


int DVBCoreTunerGetSignalStrengthQuality(TunerHandle handle,int *pStrength, int *pQuality);


/**
 *   @brief           ��ȡ�ض�tuner���ź������Ϣ
 *   @param[in]    handle:��ǰ��Ҫ ��ȡ�ź���Ϣ��tuner ���
 *   @param[out]  pInfo:ָ���ź���Ϣ��ָ��
 *   @return        success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 */
int DVBCoreTunerGetSignalInfo(TunerHandle handle,TunerInfo *pInfo);


/**
 *   @brief           ȥ��ʼ���ض�tuner ����
 *   @param[in]   handle:ָ���ض���tuner ���
 *   @return         success:AW_DVBCORE_SUCCESS
 *                 failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreTunerDeInit(TunerHandle handle);

/**
 *   @brief           ��ʼ��program manager
 *   @param[in]    param:��Ҫ��ʼ��program manager�Ĳ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCorePMInit(PMInitParam *param);

/**
 *   @brief           ����program manager��callback,�Ա�֪ͨ�ҵ���Ŀ�����Ѿ���������
 *   @param[in]   callback:�ڲ���PMCmd ��Ӧ��Ϣʱ��Ҫ���õ�callback
 *   @param[in]   pUserData:Ԥ������
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCorePMSetCallback(PMCallback callback, void* pUserData);

/**
 *   @brief          ��ȡProgram ��Ϣ
 *   @param[in]    info:��Ҫ��ʼ��program �Ĳ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note      �˺���Ϊ��ѯģʽ���ϲ���Ҫ��ѯ��ȷ��EPG�����Ƿ��и���
 *
 */
int DVBCoreProgramGetInfo(PMProgramInfolist *info, int length);

/**
 *   @brief          ��ȡProgram ��Ϣ
 *   @param[out]    length:��ȡ����Program��Ϣ����
 *   @return       success:AW_DVBCORE_SUCCESS
 *                   failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note      �˺���Ϊ��ѯģʽ���ϲ���Ҫ��ѯ��ȷ��EPG���ݸ���
 *
 */
int DVBCoreGetProgramCount(int *length);

/**
 *   @brief         ��ʼ������Ŀ
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note       �˺���Ϊ�첽����������������Ŀʱ�����callback ֪ͨ�ϲ�
 *
 */
int DVBCorePMStart();

/**
 *   @brief         ֹͣ������Ŀ
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note       �˺�����ֹͣ������Ŀ��û�н������Ľ�Ŀpid ��������֪ͨ�ϲ�
 *
 */
int DVBCorePMStop();


/**
 *   @brief           ȥ��ʼ��program manager
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCorePMDeInit();

/**
 *   @brief           ��ʼ��EPG ģ��
 *   @param[in]    param:��Ҫ��ʼ��EPG �Ĳ���
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreEPGInit(EPGInitParam *param);

/**
 *   @brief           ȥ��ʼ��EPG ģ��
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreEPGDeInit();

/**
 *   @brief          ��ȡEPG ��Ϣ
 *   @param[in]    info:��Ҫ��ʼ��EPG �Ĳ���
 *   @param[in]    length:��Ҫ������EPG��Ϣ����
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note      �˺���Ϊ��ѯģʽ���ϲ���Ҫ��ѯ��ȷ��EPG�����Ƿ��и���
 *
 */
int DVBCoreEPGGetInfo(EPGInfolist *info, int length, int epg_type, unsigned int uProgramNum);


char* DVBCoreGetTime();


/**
 *   @brief          ��ȡEPG ��Ϣ
 *   @param[out]    length:��ȡ����EPG��Ϣ����
 *   @return       success:AW_DVBCORE_SUCCESS
 *                   failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note      �˺���Ϊ��ѯģʽ���ϲ���Ҫ��ѯ��ȷ��EPG���ݸ���
 *
 */
int DVBCoreEPGGetTotalLength(int *length, int epg_type, unsigned int uProgramNum);


/**
 *   @brief         ��ʼ����EPG
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreEPGStart(unsigned int uProgramNum);

int DVBCoreEPGStartAsync(int searchCount, unsigned int *pProgramNum);

int DVBCoreEPGEnd();


/**
 *   @brief         ֹͣ����EPG
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE ���ߴ�����
 *   @note
 *
 */
int DVBCoreEPGStop();

int DVBCorePMEnd();

int DVBCore_SelectDF();

int DVBCoreMiscConf(int select);

#ifdef __cplusplus
}
#endif

#endif

