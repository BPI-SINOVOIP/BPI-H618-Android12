#include "AwDVBCore.h"
#include "ProgramManager.h"
#include "DVBCoreEpg.h"
#include "DVBCoreLog.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include "AwDVBCore.h"
#include <tsdemux_vs.h>
#include <dc_types.h>
#include <dc_log.h>
#include <dc_demod.h>

typedef struct
{
    TunerInitParam param;
    int frequency;
    int bandwidth;
    int valid;
    int connected;
    pthread_mutex_t mutex;
}TunerParam;

#undef  LOG_TAG
#define LOG_TAG "AwDvbCore"

static int gCount = 0;

DC_DemodT *demod_hdr = NULL;
pthread_mutex_t demod_mutex = PTHREAD_MUTEX_INITIALIZER;

int DVBCoreTunerInit(TunerCallback callback, TunerInitParam tunerInitParam)
{
    if (!demod_hdr)
    {
        demod_hdr = DC_DemodCreate(tunerInitParam);
        DC_ASSERT(demod_hdr);
        demod_hdr->type = tunerInitParam.stdType;

        //set demux input source type
        demux_input_source_t input_source = DMX_INPUT_SOURCE_UNKNOW;
        switch(tunerInitParam.stdType)
        {
            case DVB_STD_DVB_C:
            case DVB_STD_ATSC:
                 input_source = DMX_INPUT_SOURCE_ATSC_DVB;
                 break;

            case DVB_STD_DTMB:
            default:
                 input_source = DMX_INPUT_SOURCE_DTMB;
                 break;
        }
        TSDMX_vs_setInputSource(input_source);
    }
    demod_hdr->callback = callback;
    //start_sigmonitor_thread(demod_hdr); /* singleton instance... */
    return 0;
}

int DVBCoreTunerConnectParamChange(TunerConnectParam mNewConnectParam)
{
    int is_change = 0;
    int org_symbol_rate = 0;
    int dst_symbol_rate = 0;
    int org_modulation_mode = 0;
    int dst_modulation_mode = 0;

    switch(demod_hdr->type)
    {
        case DVB_STD_DVB_C:
            org_symbol_rate = demod_hdr->connectParam->connectParam.cab.symbol_rate;
            org_modulation_mode = demod_hdr->connectParam->connectParam.cab.modulation_mode;
            dst_symbol_rate = mNewConnectParam.connectParam.cab.symbol_rate;
            dst_modulation_mode = mNewConnectParam.connectParam.cab.modulation_mode;

            if(org_symbol_rate != dst_symbol_rate ||org_modulation_mode != dst_modulation_mode)
            {
                is_change = 1;
                printf("symbol_rate[org:%d  ->  dst:%d]  or modulation_mode[org:%d -> dst:%d] change !",
                org_symbol_rate, dst_symbol_rate, org_modulation_mode, dst_modulation_mode);
            }
            break;

        default :
             break;
    }

    return is_change;
}

int DVBCoreTunerSetConnectParam(TunerHandle handle, TunerConnectParam *connectParam)
{
    demod_hdr->handle = handle;

    demod_hdr->connectParam->uiFreq = connectParam->uiFreq;

    switch(demod_hdr->type)
    {
        case DVB_STD_DVB_C:
            demod_hdr->connectParam->connectParam.cab.symbol_rate     = connectParam->connectParam.cab.symbol_rate;
            demod_hdr->connectParam->connectParam.cab.modulation_mode = connectParam->connectParam.cab.modulation_mode;
            break;

        case DVB_STD_DVB_T:
            demod_hdr->connectParam->connectParam.ter.bandwidth  = connectParam->connectParam.ter.bandwidth;
            demod_hdr->connectParam->connectParam.ter.hierarchy_information = connectParam->connectParam.ter.hierarchy_information;
            demod_hdr->connectParam->connectParam.ter.code_rate_HP = connectParam->connectParam.ter.code_rate_HP;
            demod_hdr->connectParam->connectParam.ter.code_rate_LP = connectParam->connectParam.ter.code_rate_LP;
            demod_hdr->connectParam->connectParam.ter.guard_interval = connectParam->connectParam.ter.guard_interval;
            demod_hdr->connectParam->connectParam.ter.transmission_mode = connectParam->connectParam.ter.transmission_mode;

        case DVB_STD_DTMB:
        default :
             break;
    }

    printf("demod_hdr->connectParam->uiFreq:%u", demod_hdr->connectParam->uiFreq);
    return 0;
}


static void* TunerConnectThread(void* arg)
{
    printf("TunerConnectThread  start !!!\n");
    DC_DemodT *hdr = (DC_DemodT *)arg;
    printf("-------------set freq '%u' -----------------", hdr->connectParam->uiFreq);

    DC_DemodStop(hdr);

    int ret = DC_DemodInit(hdr);
    if (ret != 0)
    {
        printf("Demod initial failure...");
        hdr->callback(TUNER_LOCK_FAIL,TUNER_ERROR_INIT_FAIL,hdr->connectParam->uiFreq);
        goto out;
    }

    ret = DC_DemodSetFrequency(hdr, hdr->connectParam->uiFreq);
    if (ret != 0)
    {
        printf("Demod set frequency(%u) failure...", hdr->connectParam->uiFreq);
        hdr->callback(TUNER_LOCK_FAIL,TUNER_ERROR_FEQ_FAIL,hdr->connectParam->uiFreq);
    }

    if (ret == 0)
        hdr->callback(TUNER_LOCK_OK,0,hdr->connectParam->uiFreq);
    out:

    printf("-------------set freq '%s' -----------------", ret == 0 ? "success" : "fail");

    pthread_exit(0);
    return 0;
}

int DVBCoreTunerConnectAsync()
{
    if(pthread_create(&demod_hdr->mThreadId, NULL, TunerConnectThread, demod_hdr) == 0)
        return 0;
    else
        return -1;
}

int DVBCoreTunerDisconnect(TunerHandle handle)
{
    DC_UNUSE(handle);
//    close_sigmonitor_thread();

    printf("DVBCoreTunerDisconnect\n");
    DC_DemodStop(demod_hdr);

    return 0;
}

/**
 *   @brief           获取特定tuner的连接状态
 *   @param[in]    handle:当前需要 获取连接状态的tuner 句柄
 *   @return        1:连接成功
 *                0 or others:未连接上
 *   @note
 */
int DVBCoreTunerGetLocked(TunerHandle handle)
{
    DC_UNUSE(handle);
    int ret = 0;
    int is_lock = 0;
    DemodWorkStatusT work_status;
    if (!demod_hdr)
    {
        printf("demod not init...");
        is_lock = 1;
        goto out;
    }

    ret = DC_DemodGetWorkStatus(demod_hdr, &work_status);
    if (ret != 0)
    {
        printf("unknow work status");
        is_lock = 0;
        goto out;
    }

    if (work_status.state != DEMOD_STATE_WORKING)
    {
        printf("not working status");
    }

    is_lock = work_status.is_lock;

out:
    return is_lock;
}

/**
 *   @brief           获取特定tuner的信号强度
 *   @param[in]    handle:当前需要 获取信号强度的tuner 句柄
 *   @param[out]  pStrength:指向信号强度的指针
 *   @param[out]  pStrength:指向信号强度的指针
 *   @return        success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 */
int DVBCoreTunerGetSignalStrengthQuality(TunerHandle handle,int *pStrength, int *pQuality)
{
    DC_UNUSE(handle);
    int ret = 0;
    DemodWorkStatusT work_status;

    if (!demod_hdr)
    {
        printf("demod not init...");
        *pStrength = 0;
        *pQuality = 0;
        goto out;
    }

    DC_DemodGetWorkStatus(demod_hdr, &work_status);
    *pStrength = work_status.strength;
    *pQuality = work_status.quality;
out:
    return ret;
}


/**
 *   @brief           去初始化特定tuner 驱动
 *   @param[in]   handle:指向特定的tuner 句柄
 *   @return         success:AW_DVBCORE_SUCCESS
 *                 failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCoreTunerDeInit(TunerHandle handle)
{
    DC_UNUSE(handle);

    /* do nothing... */
    return 0;
}

PMContext* pPMContext = NULL;
EPGContext* pEPGContext  = NULL;

/**
 *   @brief           初始化program manager
 *   @param[in]    param:需要初始化program manager的参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCorePMInit(PMInitParam *param)
{
    DC_UNUSE(param);
    // TODO: nothing do...
    return 0;
}

/**
 *   @brief           设置program manager的callback,以便通知找到节目或者已经结束查找
 *   @param[in]   callback:在产生PMCmd 对应消息时需要调用的callback
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCorePMSetCallback(PMCallback callback, void* pUserData)
{
    int err = AW_DVBCORE_SUCCESS;
    PMCallbackInternal callbackInternal;
    callbackInternal = (PMCallbackInternal)callback;
    err = PMCompSetCallback((void* )pPMContext, callbackInternal, pUserData);
    return err;
}

/**
 *   @brief          获取Program 信息
 *   @param[in]    info:需要初始化program 的参数
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为查询模式，上层需要查询来确定EPG数据是否有更新
 *
 */
int DVBCoreProgramGetInfo(PMProgramInfolist *info, int length)
{
    char proname[512] = {0,};
    if(pPMContext != NULL)
    {
        PROGRAM  *Program = NULL;
        Program = pPMContext->mTSParser->pProgram;
        PMProgramInfolist *p = info;
        int i = 0;
        int k = 0;

        while(/*(i < length) & */(Program != NULL))
        {
            memset(proname, 0x00, 512);
            if(Program->uProgramNumber == 0 || Program->uVideoPID == 0)
            {
                Program = Program->next;
                continue;
            }

            memset(p[i].ProgramName, 0x00, 512);
            p[i].ProgramNumber = Program->uProgramNumber;
            p[i].pmt_pid = Program->uPMT_PID;
            p[i].pcr_pid = Program->uPCR_PID;
            p[i].video_pid = Program->uVideoPID;

            for(k=0;k<MAX_AUDIO_NUM;k++)
            {
                p[i].audio_pid[k] = Program->uAudioPID[k];
            }

            p[i].EIT_schedule_flag = Program->EIT_schedule_flag;
            p[i].EIT_present_following_flag = Program->EIT_present_following_flag;
            p[i].ecm_pid = Program->ECM_PID;

            snprintf(proname, sizeof(proname) - 1, "%s", Program->pucProgramName);

            strcpy((char*)p[i].ProgramName, proname);

            i++;
            Program = Program->next;
        }
        if(length == i)
            printf("DVBCoreProgramGetInfo finish !");
        else
            printf("DVBCoreProgramGetInfo set count %d ,length %d! ", i, length);

        return AW_DVBCORE_SUCCESS;
    }
    else
    {
        return AW_DVBCORE_FAILURE;
    }
}

/**
 *   @brief          获取Program 信息
 *   @param[out]    length:获取到的Program信息总数
 *   @return       success:AW_DVBCORE_SUCCESS
 *                   failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note      此函数为查询模式，上层需要查询来确定EPG数据个数
 *
 */
int DVBCoreGetProgramCount(int *length)
{

    if(pPMContext != NULL)
    {
        int count = 0;
        count = pPMContext->nHasSearchedNum;
        *length = count;
        DVBCORE_LOGD("count:%d\n",count);
        return AW_DVBCORE_SUCCESS;
    }
    else
    {
        *length = 0;
        return AW_DVBCORE_FAILURE;
    }

}


/**
 *   @brief         开始搜索节目
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note       此函数为异步函数，当搜索到节目时会调用callback 通知上层
 *
 */
int DVBCorePMStart()
{
    int err1 = AW_DVBCORE_SUCCESS;
    int err2 = AW_DVBCORE_SUCCESS;

    if(pPMContext == NULL)
    {
        pPMContext = CreateProgramManager();

        if(pPMContext == NULL)
        {
            err1 = AW_DVBCORE_FAILURE;
            goto end;
        }
#if defined(USE_AW_TSDEMUX)
        pPMContext->demux_handle = TSDMX_SingletonGetInstance();
#else
        pPMContext->demux_handle = TSDMX_vs_SingletonGetInstance();
#endif

    }

    pPMContext->end = 0;
    err2 = ProgramSearchStart(pPMContext);

end:
    return (err1 | err2);
}

int DVBCorePMEnd()
{
    if(pPMContext == NULL)
        return 1;
    else
        return pPMContext->end;
}

/**
 *   @brief         停止搜索节目
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note       此函数会停止搜索节目，没有解析到的节目pid 将不会再通知上层
 *
 */
int DVBCorePMStop()
{
    int err1 = AW_DVBCORE_SUCCESS;
    int err2 = AW_DVBCORE_SUCCESS;
    if(pPMContext != NULL)
    {
        err1 = ProgramSearchStop(pPMContext);
        err2 = ProgramManagerDestroy(pPMContext);
    }

    pPMContext = NULL;
    return (err1 | err2);
}


/**
 *   @brief           去初始化program manager
 *   @return       success:AW_DVBCORE_SUCCESS
 *               failure:AW_DVBCORE_FAILURE 或者错误码
 *   @note
 *
 */
int DVBCorePMDeInit()
{
    DVBCORE_LOGD("DVBCorePMDeInit");
    int err = AW_DVBCORE_SUCCESS;
    err = ProgramManagerDestroy(pPMContext);
    return err;
}

int DVBCoreEPGInit(EPGInitParam *param)
{
    //DC_UNUSE(param);
    int err = AW_DVBCORE_SUCCESS;

    if(pEPGContext == NULL)
    {
        pEPGContext = CreateEpgManager();

        if(pEPGContext == NULL)
        {
            err = AW_DVBCORE_FAILURE;
            goto end;
        }
#if defined(USE_AW_TSDEMUX)
        pEPGContext->demux_handle = TSDMX_SingletonGetInstance();
#else
        pEPGContext->demux_handle = TSDMX_vs_SingletonGetInstance();
#endif
        if(param)
            pEPGContext->callback_i = (EPGCallbackInternal)param->pEpgCb;
    }

end:
    return err;
}

int DVBCoreEPGDeInit()
{
    DVBCORE_LOGD("DVBCoreEPGDeInit");
    int err = AW_DVBCORE_SUCCESS;
    if(pEPGContext->demux_handle)
    {
#if defined(USE_AW_TSDEMUX)
        TSDMX_Destroy(pEPGContext->demux_handle);
#else
        TSDMX_vs_Destroy(pEPGContext->demux_handle);
#endif
        pEPGContext->demux_handle = NULL;
    }
    err = EpgManagerDestroy(pEPGContext);
    pEPGContext = NULL;
    return err;
}

int DVBCoreEPGGetInfo(EPGInfolist *info, int length, int epg_type, unsigned int uProgramNum)
{
    DC_UNUSE(epg_type);
    if(info == NULL || pEPGContext == NULL)
        return -1;

    int i, j = 0;

    PROGRAM_EVENT *pPresentEvent = NULL;
    PROGRAM_EVENT *pFollowEvent = NULL;
    PROGRAM_EVENT **pProgramEvent = NULL;

    PROGRAM_EVENT *pEvent = NULL;

    pPresentEvent = pEPGContext->mTSParser->pPresentEvent;
    pFollowEvent  = pEPGContext->mTSParser->pFollowEvent;
    pProgramEvent = pEPGContext->mTSParser->pProgramEvent;

    while((j < 2) && (NULL != pPresentEvent)) //process p/f event
    {
        pEvent = ((j == 0) ? pPresentEvent : pFollowEvent);

        if(NULL == pEvent)
            break;

        info[j].service_id = pEvent->service_id;
        info[j].event_id   = pEvent->event_id;
        info[j].running_status = pEvent->running_status;
        info[j].free_CA_mode = pEvent->free_CA_mode;

        memcpy(info[j].start_time, pEvent->start_time, sizeof(pEvent->start_time));
        memcpy(info[j].duration, pEvent->duration, sizeof(pEvent->duration));
        memcpy(info[j].event_name, pEvent->event_name_char, sizeof(pEvent->event_name_char));
        memcpy(info[j].text_char, pEvent->text_char, sizeof(pEvent->text_char));
        j++;
    }

    for(i = 0; i < EIT_SCHEDULE_TABLE_ID_NUM; i++)//process schedule event
    {
        pEvent = pProgramEvent[i];
        while((j < length) &&(pEvent != NULL))
        {
            info[j].service_id = pEvent->service_id;
            info[j].event_id   = pEvent->event_id;
            info[j].running_status = pEvent->running_status;
            info[j].free_CA_mode = pEvent->free_CA_mode;

            memcpy(info[j].start_time, pEvent->start_time, sizeof(pEvent->start_time));
            memcpy(info[j].duration, pEvent->duration, sizeof(pEvent->duration));
            memcpy(info[j].event_name, pEvent->event_name_char, sizeof(pEvent->event_name_char));
            memcpy(info[j].text_char, pEvent->text_char, sizeof(pEvent->text_char));
            pEvent = pEvent->next;
            //DVBCORE_LOGD("DVBCoreEPGGetInfo,event_count %d,  event_id %d , service_id %d, start_time %s, duration %s , event_name %s !",\
                        //j, info[j].event_id, info[j].service_id, info[j].start_time, info[j].duration, info[j].event_name);
            j++;
        }
    }

    if(j == length)
        DVBCORE_LOGD("DVBCoreEPGGetInfo service_id %d finish !", uProgramNum) ;
    else
        DVBCORE_LOGW("DVBCoreEPGGetInfo error,j %d , length %d !", j, length);

    return 0;
}

int DVBCoreEPGGetTotalLength(int *length, int epg_type, unsigned int uProgramNum)
{
    DC_UNUSE(epg_type);
    DC_UNUSE(uProgramNum);

    *length = 0;
    return 0;
}

char* DVBCoreGetTime()
{
    static char *cur_time = "0";
    char* pCurTime = cur_time;
    return pCurTime;
}

int DVBCoreEPGStart(unsigned int uProgramNum)
{
    DC_UNUSE(uProgramNum);
    return 0;
}

int DVBCoreEPGStartAsync(int searchCount ,unsigned int *pProgramNum)
{
    int ret = AW_DVBCORE_SUCCESS;

    pEPGContext->mSearchCount  = searchCount;
    memcpy(pEPGContext->mProgNum, pProgramNum, sizeof(pEPGContext->mProgNum));

    printf("pEPGContext->mProgNum %d %d %d %d !", \
            pEPGContext->mProgNum[0],pEPGContext->mProgNum[1], \
            pEPGContext->mProgNum[2], pEPGContext->mProgNum[3]);

    ret = EpgMonitorStart(pEPGContext);

    return ret;
}

int DVBCoreEPGStop()
{
    int ret = AW_DVBCORE_SUCCESS;
    ret = EpgMonitorStop(pEPGContext);

    return ret;
}

int DVBCoreEPGEnd()
{
    if(pEPGContext == NULL)
        return 1;
    else
        return pEPGContext->mEpgSearchEnd;
}

#define TSCDEV_IOC_MAGIC            't'
#define TSCDRV_ANTENNA_SWITCH           _IO(TSCDEV_IOC_MAGIC,   13)

int DVBCoreMiscConf(int select)
{
    static int dmx_fd = -1;
    int ret = 0;
    if (dmx_fd == -1)
    {
        dmx_fd = open("/dev/ts0", O_RDWR, 0);
    }

    if (dmx_fd >= 0)
    {
        ioctl(dmx_fd, TSCDRV_ANTENNA_SWITCH, select);
        ret = 0;
    }
    else
    {
        printf("invalid fd.");
        ret = -1;
    }

    return ret;
}