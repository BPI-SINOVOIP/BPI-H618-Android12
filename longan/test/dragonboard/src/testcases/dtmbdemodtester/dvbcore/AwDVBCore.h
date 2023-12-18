/**@file
*    @brief        DVBCore头文件
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
/*CA信息*/
#define CA_MAX_NAME_LENGTH                             20
#define CA_MJD_UTC_CODING_LENGTH                           5
#define MAX_PRODUCT_RECORD_COUNT                    500
#define MAX_PACKAGE_RECORD_COUNT                    10
#define SN_LENGTH                                   16
// 用户查看授权使用的数据结构
struct info_product_
{
    unsigned short    m_id;
    unsigned char     m_type;
    unsigned char      m_nameLength;
    unsigned char      m_name[CA_MAX_NAME_LENGTH];
    unsigned int      m_expired[CA_MJD_UTC_CODING_LENGTH];           // 产品失效日期，MJD
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

// 用户查看智能卡基本信息使用的数据结构
struct info_user_privacy_
{
    unsigned char     m_version;                                  // 系统版本(CA系统版本)
    unsigned char     m_serialNumber[SN_LENGTH];                  // 智能卡卡号
    unsigned char     m_algorithmFamily;                          // 智能卡所支持的算法版本群
    unsigned int    m_superCASID;                               // 当前CA系统号
    unsigned char     m_pinAvailabilityFlag;                      // 密码控制标志
    unsigned char     m_defaultPINFlag;                           // 默认PIN码标识位:1为默认PIN（默认值），0为（自定义）
    unsigned char     m_systemApprovalCertificate;                // 系统许可成人级别
    unsigned char     m_userState;                                // 用户状态（1禁用）（0启用）
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
 *   @brief           初始化特定tuner 驱动 并得到此tuner 的操作句柄
 *   @param[in]    param:需要初始化tuner 的参数
 *   @param[out]   handle:指向当前初始化的tuner 句柄
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreTunerInit(TunerCallback callback, TunerInitParam tunerInitParam);

int DVBCoreTunerConnectParamChange(TunerConnectParam mNewConnectParam);

/**
 *   @brief           配置特定tuner 的连接参数
 *   @param[in]    handle:当前需要配置连接参数的tuner 句柄
 *   @param[in]   connectParam:连接参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *                  failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreTunerSetConnectParam(TunerHandle handle,TunerConnectParam *connectParam);

/**
 *   @brief           尝试连接特定tuner
 *   @param[in]    handle:当前需要 尝试连接的tuner 句柄
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note       此函数为异步函数，马上返回。需要调用DVBCoreTunerGetLocked去查询
 *               特定tuner的连接状态
 *
 */
int DVBCoreTunerConnectAsync();

/**
 *   @brief           断开与特定tuner的连接
 *   @param[in]    handle:当前需要 断开连接的tuner 句柄
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为同步函数
 *
 */
int DVBCoreTunerDisconnect(TunerHandle handle);


/**
 *   @brief           获取特定tuner的连接状态
 *   @param[in]    handle:当前需要 获取连接状态的tuner 句柄
 *   @return        1:连接成功
 *                0 or others:未连接上
 *   @note
 */
int DVBCoreTunerGetLocked(TunerHandle handle);


int DVBCoreTunerGetSignalStrengthQuality(TunerHandle handle,int *pStrength, int *pQuality);


/**
 *   @brief           获取特定tuner的信号相关信息
 *   @param[in]    handle:当前需要 获取信号信息的tuner 句柄
 *   @param[out]  pInfo:指向信号信息的指针
 *   @return        success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 */
int DVBCoreTunerGetSignalInfo(TunerHandle handle,TunerInfo *pInfo);


/**
 *   @brief           去初始化特定tuner 驱动
 *   @param[in]   handle:指向特定的tuner 句柄
 *   @return         success:AW_DVBCORE_SUCCESS
 *                 failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreTunerDeInit(TunerHandle handle);

/**
 *   @brief           初始化program manager
 *   @param[in]    param:需要初始化program manager的参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCorePMInit(PMInitParam *param);

/**
 *   @brief           设置program manager的callback,以便通知找到节目或者已经结束查找
 *   @param[in]   callback:在产生PMCmd 对应消息时需要调用的callback
 *   @param[in]   pUserData:预留参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCorePMSetCallback(PMCallback callback, void* pUserData);

/**
 *   @brief          获取Program 信息
 *   @param[in]    info:需要初始化program 的参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为查询模式，上层需要查询来确定EPG数据是否有更新
 *
 */
int DVBCoreProgramGetInfo(PMProgramInfolist *info, int length);

/**
 *   @brief          获取Program 信息
 *   @param[out]    length:获取到的Program信息总数
 *   @return       success:AW_DVBCORE_SUCCESS
 *                   failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为查询模式，上层需要查询来确定EPG数据个数
 *
 */
int DVBCoreGetProgramCount(int *length);

/**
 *   @brief         开始搜索节目
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note       此函数为异步函数，当搜索到节目时会调用callback 通知上层
 *
 */
int DVBCorePMStart();

/**
 *   @brief         停止搜索节目
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note       此函数会停止搜索节目，没有解析到的节目pid 将不会再通知上层
 *
 */
int DVBCorePMStop();


/**
 *   @brief           去初始化program manager
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCorePMDeInit();

/**
 *   @brief           初始化EPG 模块
 *   @param[in]    param:需要初始化EPG 的参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreEPGInit(EPGInitParam *param);

/**
 *   @brief           去初始化EPG 模块
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreEPGDeInit();

/**
 *   @brief          获取EPG 信息
 *   @param[in]    info:需要初始化EPG 的参数
 *   @param[in]    length:需要拷贝的EPG信息个数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为查询模式，上层需要查询来确定EPG数据是否有更新
 *
 */
int DVBCoreEPGGetInfo(EPGInfolist *info, int length, int epg_type, unsigned int uProgramNum);


char* DVBCoreGetTime();


/**
 *   @brief          获取EPG 信息
 *   @param[out]    length:获取到的EPG信息总数
 *   @return       success:AW_DVBCORE_SUCCESS
 *                   failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为查询模式，上层需要查询来确定EPG数据个数
 *
 */
int DVBCoreEPGGetTotalLength(int *length, int epg_type, unsigned int uProgramNum);


/**
 *   @brief         开始解析EPG
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreEPGStart(unsigned int uProgramNum);

int DVBCoreEPGStartAsync(int searchCount, unsigned int *pProgramNum);

int DVBCoreEPGEnd();


/**
 *   @brief         停止解析EPG
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
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

