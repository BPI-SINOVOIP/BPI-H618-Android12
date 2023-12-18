
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>

#include "aw_dvbcore.h"
#include "dc_types.h"
#include "dc_log.h"
#include "AwDVBCore.h"
#include <awesome_pgsearch.h>

struct DVBCoreS
{
    int mTunerHandle;
    int mSPStreamHandle;
    int mEpgSearchFlag;
    int mEpgSearchedValidCount;
    int freq;
    int mPgsStartFlag;

    pthread_t mStartThreadId;
    pthread_t mEpgSearchThreadId;
    pthread_mutex_t mStateMutex;

    DvbStateT    mDvbState;
    DvbControlCmdT mControlCmd;

    DCListenerT *DCListener;
    ProgramIDT mProgramID;
    ProgramInfolistT *pProgramInfolist;
    EPGSearchParamT   mEpgParam;
    PGShandlerT *pgs;

};

DVBCoreT *pDVBCore = NULL;

void epg_search_callback_i(int status, int event_count, void *param);
void cas_event_func(unsigned int event,unsigned int param1,unsigned int param2,unsigned int param3);
void tuner_event_callback(unsigned int event,unsigned int param1,unsigned int param2);

static int stop_search_program(void *cookie)
{
    printf("process stop search program !");

    DVBCoreT *dc = (DVBCoreT*)cookie;

    if(dc->pgs != NULL && dc->mDvbState == DVB_STATE_SEARCHING_PROGRAM)
    {
        while(1)
        {
            if(dc->mDvbState == DVB_STATE_INITED || dc->mPgsStartFlag == 1)
            {
                PGS_Stop(dc->pgs);
                dc->mPgsStartFlag = 0;
                printf("stop search program finish !");
                break;
            }
            usleep(10 *1000);
        }
    }

    return 0;
}

static int stop_search_epg(void *cookie)
{
    printf("process stop search epg !");

    DVBCoreT *dc = (DVBCoreT*)cookie;
    if(!dc->mEpgSearchFlag)
    {
        printf("epg search thread not start ,return !");
        return 0;
    }

    if(dc->mDvbState == DVB_STATE_SEARCHING_EPG)
    {
        pthread_mutex_lock(&dc->mStateMutex);
        dc->mDvbState = DVB_STATE_INITED;
        pthread_mutex_unlock(&dc->mStateMutex);
        printf("dvb state changed to %d !", dc->mDvbState);
    }

    if(dc->mEpgSearchThreadId)
    {
        pthread_join(dc->mEpgSearchThreadId, NULL);
        printf("wait epg search thread exit finish!");
    }

    return 0;
}


static int get_epg_info(void *cookie,int event_num,int serviceId)
{
    printf("get_epg_info !");

    int ret = 0;
    int i = 0;
    EPGInfoT  *pEpgInfos = NULL;
    EPGInfolist *pEpgInfolist = NULL;

    DVBCoreT *dc = (DVBCoreT*)cookie;

    EpgEventInfolist mEpgEventInfolist;
    memset(&mEpgEventInfolist, 0x00, sizeof(EpgEventInfolist));
    mEpgEventInfolist.service_id  = serviceId;
    mEpgEventInfolist.search_freq = dc->mEpgParam.SearchFreq;
    mEpgEventInfolist.event_count = event_num;

    if(event_num < 1)
    {
        printf("search epg serviceId %d no result!", serviceId);
        if(dc->DCListener->onEPGSearch)
            dc->DCListener->onEPGSearch(NULL, DC_STATUS_ERR, &mEpgEventInfolist);

        dc->mEpgSearchedValidCount++;

        return -1;
    }

    pEpgInfos = (EPGInfoT *)malloc(sizeof(EPGInfoT) * event_num);
    DC_ASSERT(pEpgInfos);
    memset(pEpgInfos, 0x00, sizeof(EPGInfoT) * event_num);

    pEpgInfolist = (EPGInfolist *)malloc(sizeof(EPGInfolist) * event_num);
    DC_ASSERT(pEpgInfolist);
    memset(pEpgInfolist, 0x00, sizeof(EPGInfolist) * event_num);
    ret =  DVBCoreEPGGetInfo(pEpgInfolist, event_num, 0 , serviceId);

    for(; i < event_num; i++)
    {
        pEpgInfos[i].event_id   = pEpgInfolist[i].event_id;
        memcpy(pEpgInfos[i].start_time, pEpgInfolist[i].start_time, sizeof(pEpgInfolist[i].start_time));
        memcpy(pEpgInfos[i].duration, pEpgInfolist[i].duration, sizeof(pEpgInfolist[i].duration));
        memcpy(pEpgInfos[i].event_name, pEpgInfolist[i].event_name, sizeof(pEpgInfolist[i].event_name));
        memcpy(pEpgInfos[i].text_char, pEpgInfolist[i].text_char, sizeof(pEpgInfolist[i].text_char));
        if(i < event_num - 1)
            pEpgInfos[i].next = &pEpgInfos[i+1];
        else
            pEpgInfos[i].next = NULL;

        //printf("event_count %d, get_epg_info, event_id %d , serviceId %d , start_time %s ,end_time %s !",\
            //i, pEpgInfos[i].event_id, pEpgInfos[i].service_id, pEpgInfos[i].start_time, pEpgInfos[i].duration);
    }

    dc->mEpgSearchedValidCount++;
    printf("search epg count %d !", dc->mEpgSearchedValidCount);

#if 0
    while(i < event_num && pEpgInfos != NULL)
    {
        printf("event_count %d, event_id %d , serviceId %d , start_time %s ,end_time %s !",\
                i, pEpgInfos->event_id, pEpgInfos->service_id, pEpgInfos->start_time, pEpgInfos->duration);
        pEpgInfos = pEpgInfos->next;
        i++;
    }
#endif
    mEpgEventInfolist.pInfos = pEpgInfos;

    if(dc->DCListener->onEPGSearch && dc->mControlCmd != STOP_SEARCH_EPG_CMD)
        dc->DCListener->onEPGSearch(NULL, DC_STATUS_OK, &mEpgEventInfolist);

    if(pEpgInfolist)
    {
        free(pEpgInfolist);
        pEpgInfolist = NULL;
    }

    if(pEpgInfos)
    {
        free(pEpgInfos);
        pEpgInfos = NULL;
    }

    return ret;
}

static int __PGS_OnFinish(void *cookie, int err, struct ProgramInfoS *program_info, int program_num)
{
    printf("OnFinish., err:'%d' program:'%d'", err, program_num);

    struct DVBCoreS *dc = cookie;

    int sig_strength;
    int sig_quality;

    DVBCoreTunerGetSignalStrengthQuality(dc->mTunerHandle,&sig_strength, &sig_quality);

    ProgramInfolistT  *prgram_info_list = (ProgramInfolistT *)malloc(sizeof(ProgramInfolistT) * program_num);
    DC_ASSERT(prgram_info_list);

    memset(prgram_info_list, 0x00, sizeof(ProgramInfolistT)*program_num);

    prgram_info_list->ValidProgramCount = 0;
    int i = 0;
    for (; i<program_num; i++)
    {

        printf("[%d]pmt_pid:'%d', name:'%s'", i, program_info[i].pmt_pid, program_info[i].name);
        if (!program_info[i].video_pid)
        {
            printf("not valid program ,ProgramNumber %d !", program_info[i].index);
            continue;
        }

        prgram_info_list[i].id.freq    = dc->freq;
        prgram_info_list[i].id.pmt_pid = program_info[i].pmt_pid;
        prgram_info_list[i].id.pcr_pid = PROGRAM_INVALID_PID;
        prgram_info_list[i].id.video_pid = program_info[i].video_pid;
        prgram_info_list[i].id.signal.quality = sig_quality;
        prgram_info_list[i].id.signal.strength = sig_strength;
        prgram_info_list[i].id.scrambled = program_info[i].scrambled;
        prgram_info_list[i].id.free_CA_mode = program_info[i].free_CA_mode;
        prgram_info_list[i].id.running_status = program_info[i].running_status;
        prgram_info_list[i].EIT_schedule_flag = program_info[i].EIT_schedule_flag;
        prgram_info_list[i].EIT_present_following_flag = program_info[i].EIT_present_following_flag;
        int index;

        for(index = 0; index < program_info[i].audio_num; index++)
        {
            prgram_info_list[i].id.audio_pid[index] = program_info[i].audio_pid[index];
            printf("audio pid is %d ,index %d !", prgram_info_list[i].id.audio_pid[index], index);
        }

        for(index = program_info[i].audio_num; index < MAX_AUDIO_NUM; index++)
        {
            prgram_info_list[i].id.audio_pid[index] = PROGRAM_INVALID_PID;
        }

        prgram_info_list[i].id.ProgramNumber = program_info[i].index;

        char name_str[512] = {0};
        sprintf(name_str, "xxx%s", program_info[i].name);
        for(uint8_t j = 0;j < program_info[i].multi_name_loop_count;j++)
        {
            sprintf(name_str, "%s|%.3s%s", name_str, program_info[i].multi_name_loop[j].multi_language, program_info[i].multi_name_loop[j].multi_name);
        }
        strcpy(prgram_info_list[i].ProgramName, name_str);

        prgram_info_list->ValidProgramCount++;

        printf("%s ,%d ,%d ,%d ,%d ,%d ,%d, %d, %d", prgram_info_list[i].ProgramName, prgram_info_list[i].id.ProgramNumber, prgram_info_list[i].id.freq, prgram_info_list[i].id.pmt_pid, prgram_info_list[i].id.video_pid, prgram_info_list[i].id.signal.quality, prgram_info_list[i].id.signal.strength, prgram_info_list[i].id.scrambled, prgram_info_list[i].id.free_CA_mode);
    }

    if(dc->DCListener->onTryFreqFinish && dc->mControlCmd != STOP_SEARCH_PROGRAM_CMD)
    {
        if(prgram_info_list && prgram_info_list->ValidProgramCount > 0)
        {
            printf("get valid program count %d !", prgram_info_list->ValidProgramCount);
            dc->DCListener->onTryFreqFinish(NULL, DC_STATUS_OK, (void*)prgram_info_list);
        }
        else
        {
            dc->DCListener->onTryFreqFinish(NULL, DC_STATUS_ERR, (void *)&dc->freq);
        }
    }

    if(prgram_info_list)
        free(prgram_info_list);

    pthread_mutex_lock(&dc->mStateMutex);
    dc->mDvbState = DVB_STATE_INITED;
    pthread_mutex_unlock(&dc->mStateMutex);

    return 0;
}

struct PGS_ListenerS pgs_listener =
{
    .onFinish = __PGS_OnFinish
};

static int lockTuner(TunerConnectParam mConnectParam)
{
    DVBCoreTunerSetConnectParam(pDVBCore->mTunerHandle, &mConnectParam);
    return DVBCoreTunerConnectAsync();
}

static void* epgSearchThread(void* arg)
{
    int ret;
    int i;
    DVBCoreT *dc = (DVBCoreT*)arg;

    for(i = dc->mEpgParam.SearchCount; i < MAX_EPG_SEARCH_NUM; i++)
        dc->mEpgParam.ServiceID[i] = 0;

    ret = DVBCoreEPGStartAsync(dc->mEpgParam.SearchCount, dc->mEpgParam.ServiceID);

    while(!DVBCoreEPGEnd() && (dc->mDvbState != DVB_STATE_INITED))
    {
        //printf("############wait epg search end !");
        usleep(100 * 1000);
    }

    ret = DVBCoreEPGStop();
    ret = DVBCoreEPGDeInit();

    if(dc->mDvbState == DVB_STATE_SEARCHING_EPG)
    {
        pthread_mutex_lock(&dc->mStateMutex);
        dc->mDvbState = DVB_STATE_INITED;
        pthread_mutex_unlock(&dc->mStateMutex);
    }

    dc->mEpgSearchFlag = 0;
    dc->mEpgSearchedValidCount = 0;

    pthread_exit(NULL);
    return 0;
}

static void* StartThread(void* arg)
{
    DVBCoreT *dc = (DVBCoreT*)arg;

    int i, ret;
    int lock_status, lost_count = 0, lost_signal = 0;
    char *pStreamUrl = NULL;
    SPStreamParam mStParam;
    DvbNotifyMsgT mMessage;

    mStParam.freq    = dc->mProgramID.freq;
    mStParam.pid_pcr = (short)dc->mProgramID.pcr_pid;
    mStParam.pid_pmt = (short)dc->mProgramID.pmt_pid;
    mStParam.pid_video = (short)dc->mProgramID.video_pid;

    for(i = 0; i< MAX_AUDIO_NUM; i++)
    {
        mStParam.pid_audio[i] = (short)dc->mProgramID.audio_pid[i];
    }
#if 0
    ret = DVBCoreSPCreateStream(&mStParam,&dc->mSPStreamHandle);
    if(ret < 0)
        goto start_error;

    ret = DVBCoreSPStartStream(dc->mSPStreamHandle);
    if(ret < 0)
        goto start_error;

    pStreamUrl =  DVBCoreSPGetStreamUrl(dc->mSPStreamHandle);
    if( pStreamUrl == NULL)
        goto start_error;
#endif
    sprintf(pStreamUrl, "%s#program_number=0x%x", pStreamUrl, dc->mProgramID.ProgramNumber);

    if(dc->DCListener->onStartFinish)
    {
        printf("get stream url %s finish !", pStreamUrl);
        dc->DCListener->onStartFinish(NULL, DC_STATUS_OK, pStreamUrl);
    }

    while(dc->mDvbState == DVB_STATE_STARTD)
    {
        mMessage = DVB_UNKNOWED_MSG;
        lock_status = DVBCoreTunerGetLocked(dc->mTunerHandle);

        if(lock_status && lost_signal)
        {
            lost_signal = 0;
            lost_count  = 0;
            mMessage = DVB_RECOVER_SIGNAL;
            printf("freq %d found signal ,notify !", dc->mProgramID.freq);
        }
        else if(!lock_status && (lost_count < MAX_LOST_SIGNAL_CNT))
        {
            lost_count++;
            if(lost_count >= MAX_LOST_SIGNAL_CNT)
            {
                lost_signal = 1;
                mMessage = DVB_LOST_SIGNAL;
                printf("freq %d lost signal count %d ,notify!", dc->mProgramID.freq, lost_count);
            }
        }

        if(dc->DCListener->onNotify && mMessage != DVB_UNKNOWED_MSG)
        {
            dc->DCListener->onNotify(NULL, mMessage, (void *)&dc->mProgramID.freq);
        }

        usleep(100 * 1000);
    }

     goto thread_exit;

start_error:
    if(dc->mSPStreamHandle != -1)
    {
        //DVBCoreSPDestroyStream(dc->mSPStreamHandle);
        dc->mSPStreamHandle = -1;
    }

    if(dc->DCListener->onStartFinish)
    {
        printf("start stream error !");
        pthread_mutex_lock(&dc->mStateMutex);
        dc->mDvbState = DVB_STATE_INITED;
        pthread_mutex_unlock(&dc->mStateMutex);
        dc->DCListener->onStartFinish(NULL, DC_STATUS_ERR, NULL);
    }

thread_exit:
    pthread_exit(NULL);
    return 0;
}

static int create_start_thread(void* arg)
{
    DVBCoreT *dc = (DVBCoreT*)arg;

    if(pthread_create(&dc->mStartThreadId, NULL, StartThread, dc) == 0)
    {
        printf("[Aw_DvbCore] Start Thread create finish ! \n");
        return AW_DVBCORE_SUCCESS;
    }
    else
    {
        printf("[Aw_DvbCore] start failed!");
        return AW_DVBCORE_FAILURE;
    }
}

static int create_epg_thread(void* arg)
{
    DVBCoreT *dc = (DVBCoreT*)arg;

    if(pthread_create(&dc->mEpgSearchThreadId, NULL, epgSearchThread, dc) == 0)
    {
        printf("[Aw_DvbCore] Epg search thread create finish ! \n");
        return  AW_DVBCORE_SUCCESS;
    }
    else
    {
        printf("[Aw_DvbCore] Epg search failed!");
        return  AW_DVBCORE_FAILURE;
    }
}

void epg_search_callback_i(int status, int event_count, void *param)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);
    int service_id = -1;
    int event_num = event_count;
    if(param)
    {
        service_id = *((int*)param);
        printf("event %d, status %d, param %d", event_count, status, service_id);
    }

    switch(status)
    {
        case 0:
        case 1:
            get_epg_info(pDVBCore,event_num,service_id);
            break;
    }

    printf("epg search fininsh ,need search %d ,searched vaild count %d !", \
            pDVBCore->mEpgParam.SearchCount, pDVBCore->mEpgSearchedValidCount);

    return;
}

void cas_event_func(unsigned int event,unsigned int param1,unsigned int param2,unsigned int param3)
{
    //TODO*
    DC_UNUSE(event);
    DC_UNUSE(param1);
    DC_UNUSE(param2);
    DC_UNUSE(param3);

    return;
}

void tuner_event_callback(unsigned int event,unsigned int param1,unsigned int param2)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    int lock_count = 0;
    int sig_strength = 0;
    int sig_quality  = 0;
    int lock_result = event;

    DVBCoreT *dc = pDVBCore;

    printf("event %d, status %d , freq %d !", event, param1, param2);

    //TODO*
    switch(event)
    {
        case TUNER_LOCK_FAIL:
            printf("lock %u fail...", param2);
            break;

        case TUNER_LOCK_OK:
            while (DVBCoreTunerGetLocked(dc->mTunerHandle) == 0)
            {
                if(dc->mControlCmd == STOP_SEARCH_PROGRAM_CMD)
                {
                    lock_result = TUNER_LOCK_FAIL;
                    break;
                }

                DVBCoreTunerGetSignalStrengthQuality(dc->mTunerHandle,&sig_strength, &sig_quality);
                printf("freq %u in unlock status, sig_strength %d ,sig_quality %d !", param2, sig_strength, sig_quality);

                if(++lock_count > 5)
                {
                    lock_result = TUNER_LOCK_FAIL;
                    break;
                }

                usleep(1 * 1000 * 1000);
            }
            break;
    }

    printf("lock freq %u  result %d !", param2, lock_result);

    if(lock_result)
    {
        if(dc->mDvbState == DVB_STATE_SEARCHING_PROGRAM)
        {
            if(dc->mControlCmd == STOP_SEARCH_PROGRAM_CMD)
            {
                printf("lock ok, process stop program cmd !");
                pthread_mutex_lock(&dc->mStateMutex);
                dc->mDvbState = DVB_STATE_INITED;
                pthread_mutex_unlock(&dc->mStateMutex);
            }
            else
            {
                PGS_Start(dc->pgs);
                dc->mPgsStartFlag = 1;
            }
        }
        else if(dc->mDvbState == DVB_STATE_STARTD)
        {
            create_start_thread(dc);
        }
        else if(dc->mDvbState == DVB_STATE_SEARCHING_EPG)
        {
            create_epg_thread(dc);
        }
        else
        {
            printf("state error, current state %d !", dc->mDvbState);
        }
    }
    else //process locked fail callback
    {
        DvbStateT mLastState = dc->mDvbState;

        pthread_mutex_lock(&dc->mStateMutex);
        dc->mDvbState = DVB_STATE_INITED;
        pthread_mutex_unlock(&dc->mStateMutex);

        if(mLastState == DVB_STATE_STARTD)
        {
            dc->DCListener->onStartFinish(NULL, DC_STATUS_ERR, NULL/* param */);
        }
        else if(mLastState == DVB_STATE_SEARCHING_PROGRAM &&
                dc->mControlCmd != STOP_SEARCH_PROGRAM_CMD)
        {
            dc->DCListener->onTryFreqFinish(NULL, DC_STATUS_ERR, (void *)&param2/* param */);
        }
        else if(mLastState == DVB_STATE_SEARCHING_EPG)
        {
            dc->DCListener->onEPGSearch(NULL, DC_STATUS_ERR, NULL);
        }
    }

    return;
}

DVBCoreT *DVBCoreCreate(DCListenerT *listener, DvbStandardType nStandardType)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    int ret = AW_DVBCORE_SUCCESS;

    if(pDVBCore != NULL)
    {
        printf("[Aw_DvbCore] already create %p! \n", pDVBCore);
        return pDVBCore;
    }

    DVBCoreT *dc = (DVBCoreT*)calloc(1, sizeof(DVBCoreT));
    if (dc == NULL)
        return NULL;

    dc->DCListener = NULL;

    if(listener)
    {
        dc->DCListener = (DCListenerT*)calloc(1, sizeof(DCListenerT));
        if (dc->DCListener == NULL)
            goto create_error;
        else
            memcpy(dc->DCListener, listener, sizeof(DCListenerT));
    }

    //create demod instance
    TunerInitParam mTunerInitParam;
    memset(&mTunerInitParam, 0x00 , sizeof(TunerInitParam));
    mTunerInitParam.stdType = nStandardType;
    ret = DVBCoreTunerInit(tuner_event_callback, mTunerInitParam);
    if(ret != AW_DVBCORE_SUCCESS)
    {
        printf("DVBCoreTunerInit failed.");
        goto create_error;
    }

    //create demux instance
    //DVBCoreSPInit(NULL);

    pthread_mutex_init(&dc->mStateMutex, NULL);
    dc->mSPStreamHandle = -1;
    dc->mDvbState = DVB_STATE_UNINITED;

    pDVBCore = dc;
    printf("[Aw_DvbCore] create finish %p ! \n", pDVBCore);

    return dc;

create_error:
    if(dc->DCListener)
    {
        free(dc->DCListener);
        dc->DCListener = NULL;
    }
    if(dc)
    {
        free(dc);
        dc = NULL;
        pDVBCore = NULL;
    }
    return NULL;
}

int DVBCoreInit(DVBCoreT *dc, void* param)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    DC_UNUSE(param);
    int ret = AW_DVBCORE_SUCCESS;

    if(dc == NULL  || dc->mDvbState != DVB_STATE_UNINITED /*|| param == NULL*/)
    {
        printf("[Aw_DvbCore] DVBCoreInit failed, state %d !", dc->mDvbState);
        return AW_DVBCORE_FAILURE;
    }

    dc->mControlCmd = UNKNOWN_CONTROL_CMD;

    dc->mDvbState = DVB_STATE_INITED;
    printf("DVBCoreInit finish.");

    return ret;
}

/*
*  定点搜频，异步调用，完成之后通过onTryFreqFinish通知用户，并且把该频点节目信息回传给用户。
*/
int DVBCoreTryFreqAsync(DVBCoreT *dc, TunerConnectParam mConnectParam)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    int freq = 0;
    int need_lock = 0;
    int ret = AW_DVBCORE_SUCCESS;

    if(dc == NULL || dc->mDvbState != DVB_STATE_INITED)
    {
        printf("[Aw_DvbCore] DVBCorePrepareAsync failed, state %d !", dc->mDvbState);
        return AW_DVBCORE_FAILURE;
    }

    freq = mConnectParam.uiFreq;
    if(freq <= 0)
    {
        printf("[Aw_DvbCore] DVBCoreTryFreqAsync failed, please check freq %d !", freq);
        return AW_DVBCORE_FAILURE;
    }

    if(freq != dc->freq || DVBCoreTunerConnectParamChange(mConnectParam)||
       !DVBCoreTunerGetLocked(dc->mTunerHandle))
    {
        printf("freq changed or tuner locked %d ,lock tuner !", DVBCoreTunerGetLocked(dc->mTunerHandle));
        need_lock = 1;
    }

    dc->freq = freq;

    if(dc->pgs)
    {
        PGS_Stop(dc->pgs);
        PGS_Destroy(dc->pgs);
        dc->mPgsStartFlag = 0;
        dc->pgs  = NULL;
        printf("destroy pgs instance about last search!");
    }

    dc->pgs = PGS_Instance(&pgs_listener, dc, freq, 0);

    if(need_lock)
    {
        ret = lockTuner(mConnectParam);
    }
    else
    {
        printf("freq not changed and tuner locked %d ,pgs start !", DVBCoreTunerGetLocked(dc->mTunerHandle));
        ret = PGS_Start(dc->pgs);
        dc->mPgsStartFlag = 1;
    }

    if(ret < 0)
    {
        printf("DVBCoreTunerConnectAsync error...");
        return AW_DVBCORE_FAILURE;
    }

    pthread_mutex_lock(&dc->mStateMutex);
    dc->mDvbState = DVB_STATE_SEARCHING_PROGRAM;
    pthread_mutex_unlock(&dc->mStateMutex);

    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    return ret;
}

int DVBCoreStartAsync(DVBCoreT *dc, TunerConnectParam mConnectParam, ProgramIDT mProgramID)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    int need_lock = 0;
    int ret = AW_DVBCORE_SUCCESS;

    if(dc == NULL || dc->mDvbState != DVB_STATE_INITED)
    {
        printf("[Aw_DvbCore] DVBCoreStart failed, state %d !", dc->mDvbState);
        return AW_DVBCORE_FAILURE;
    }

    //make sure audio or video pid valid
    if(mProgramID.freq <= 0 || (mProgramID.video_pid <= 0 && mProgramID.audio_pid[0] == PROGRAM_INVALID_PID))
    {
        printf("[Aw_DvbCore] DVBCoreStart, please check freq %d ,video pid %d !,audio pid %d",
                mProgramID.freq, mProgramID.video_pid, mProgramID.audio_pid[0]);
        return AW_DVBCORE_FAILURE;
    }

    if(mProgramID.freq != dc->freq || !DVBCoreTunerGetLocked(dc->mTunerHandle))
    {
        printf("freq changed or tuner locked %d ,lock tuner !", DVBCoreTunerGetLocked(dc->mTunerHandle));
        need_lock = 1;
    }

    dc->freq = mProgramID.freq;
    memcpy(&dc->mProgramID, &mProgramID, sizeof(ProgramIDT));

    if(need_lock)
    {
        mConnectParam.uiFreq = mProgramID.freq;
        ret = lockTuner(mConnectParam);
    }
    else
    {
        printf("freq not changed and tuner locked %d ,start !", DVBCoreTunerGetLocked(dc->mTunerHandle));
        ret = create_start_thread(dc);
    }

    if(ret < 0)
        return AW_DVBCORE_FAILURE;

    pthread_mutex_lock(&dc->mStateMutex);
    dc->mDvbState = DVB_STATE_STARTD;
    pthread_mutex_unlock(&dc->mStateMutex);

    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    return ret;
}

int DVBCoreEPGSearchAsync(DVBCoreT *dc, EPGSearchParamT mSearchParam)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    int need_lock = 0;
    int ret = AW_DVBCORE_SUCCESS;

    if(dc == NULL || (dc->mDvbState != DVB_STATE_INITED && dc->mDvbState != DVB_STATE_STARTD))
    {
        printf("[Aw_DvbCore] DVBCoreEPGSearchAsync failed, state is %d !", dc->mDvbState);
        return AW_DVBCORE_FAILURE;
    }

    if(mSearchParam.SearchFreq <= 0)
    {
        printf("[Aw_DvbCore] DVBCoreEPGSearchAsync, please check freq %d !", mSearchParam.SearchFreq);
        return AW_DVBCORE_FAILURE;
    }

    if((dc->mDvbState == DVB_STATE_STARTD) &&  (dc->freq != mSearchParam.SearchFreq))
    {
        printf("[Aw_DvbCore] EPG search freq not match [need: %d,current: %d]!", mSearchParam.SearchFreq ,dc->freq);
        return AW_DVBCORE_FAILURE;
    }

    if(dc->mDvbState == DVB_STATE_INITED)
    {
        need_lock = 1;
    }

    //init epg
    EPGInitParam EpgInitParam;
    EpgInitParam.pEpgCb = epg_search_callback_i;
    DVBCoreEPGInit(&EpgInitParam);

    dc->mEpgSearchFlag = 1;
    memcpy(&dc->mEpgParam, &mSearchParam, sizeof(EPGSearchParamT));

    if(need_lock)
    {
        TunerConnectParam mConnectParam;
        memset(&mConnectParam, 0x00, sizeof(TunerConnectParam));
        mConnectParam.uiFreq = mSearchParam.SearchFreq;

        ret = lockTuner(mConnectParam);
        printf("Epg start in inited state %d !", dc->mDvbState);
    }
    else
    {
        printf(" Epg start in started state %d !", dc->mDvbState);
        ret = create_epg_thread(dc);
    }

    if(ret < 0)
    {
        DVBCoreEPGDeInit();
        return AW_DVBCORE_FAILURE;
    }

    if(dc->mDvbState == DVB_STATE_INITED)
    {
        pthread_mutex_lock(&dc->mStateMutex);
        dc->mDvbState = DVB_STATE_SEARCHING_EPG;
        pthread_mutex_unlock(&dc->mStateMutex);
    }

    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    return ret;
}

int DVBCoreStop(DVBCoreT *dc)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    int ret = AW_DVBCORE_SUCCESS;

    if(dc->mDvbState == DVB_STATE_STARTD)
    {
        pthread_mutex_lock(&dc->mStateMutex);
        dc->mDvbState = DVB_STATE_INITED;
        pthread_mutex_unlock(&dc->mStateMutex);
    }

    if(dc->mStartThreadId)
    {
        pthread_join(dc->mStartThreadId, NULL);
        printf("[Aw_DvbCore] wait start thread  exit ok !");
    }

    if(dc->mSPStreamHandle != -1)
    {
        //ret = DVBCoreSPStopStream(dc->mSPStreamHandle);
        //ret = DVBCoreSPDestroyStream(dc->mSPStreamHandle);
        dc->mSPStreamHandle = -1;
    }

    if(dc->mEpgSearchFlag)
    {
        ret = stop_search_epg(dc);
    }

    printf("[Aw_DvbCore] stop finish ! \n");

    return ret;
}

/* 获取当前所锁频的信号质量，如未锁频，对应数值为0 */
int DVBCoreGetSignalInfo(DVBCoreT *dc, SignalInfoT *si)
{
    if(si == NULL)
        return AW_DVBCORE_FAILURE;

    return DVBCoreTunerGetSignalStrengthQuality(dc->mTunerHandle, &si->strength, &si->quality);
}

int DVBCoreControl(DVBCoreT *dc, DvbControlCmdT cmd, void *param)
{
    DC_UNUSE(param);

    printf("[Aw_DvbCore] %s, cmd %d !", __FUNCTION__, cmd);

    int ret = AW_DVBCORE_SUCCESS;
    if(dc == NULL)
    {
        printf("[Aw_DvbCore] DVBCoreControl failed, dc is NULL !");
        return AW_DVBCORE_FAILURE;
    }

    dc->mControlCmd = cmd;
    switch(cmd)
    {
        case STOP_SEARCH_PROGRAM_CMD:
             ret = stop_search_program(dc);
             break;

        case STOP_SEARCH_EPG_CMD:
             ret = stop_search_epg(dc);
             break;

        default:
             break;
    }

    dc->mControlCmd = UNKNOWN_CONTROL_CMD;

    return ret;
}

int  DVBCoreDeInit(DVBCoreT *dc)
{
    if(dc == NULL)
    {
        printf("[Aw_DvbCore] DVBCoreDeInit failed, dc is NULL !");
        return AW_DVBCORE_FAILURE;
    }

    if(dc->mDvbState == DVB_STATE_UNINITED)
    {
        printf("[Aw_DvbCore] DVBCoreDeInit, already in uninited state !");
        return AW_DVBCORE_FAILURE;
    }

    if(dc->mSPStreamHandle != -1)
    {
        //DVBCoreSPStopStream(dc->mSPStreamHandle);
        //DVBCoreSPDestroyStream(dc->mSPStreamHandle);
        dc->mSPStreamHandle = -1;
    }
    if(dc->pProgramInfolist)
    {
        free(dc->pProgramInfolist);
        dc->pProgramInfolist = NULL;
    }

    if(dc->pgs)
    {
        PGS_Stop(dc->pgs);
        PGS_Destroy(dc->pgs);
        dc->pgs  = NULL;
    }

    if(dc->mEpgSearchFlag)
    {
        printf("[Aw_DvbCore] deinit stop epg ! \n");
        stop_search_epg(dc);
    }

    dc->mDvbState = DVB_STATE_UNINITED;
    dc->mControlCmd = UNKNOWN_CONTROL_CMD;

    DVBCoreTunerDisconnect(pDVBCore->mTunerHandle);

    printf("[Aw_DvbCore] deinit finish ! \n");

    return 0;
}

int DVBCoreDestroy(DVBCoreT *dc)
{
    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);

    if(dc == NULL || dc->mDvbState != DVB_STATE_UNINITED)
    {
        printf("[Aw_DvbCore] DVBCoreDestroy failed, state %d !", dc->mDvbState);
        return AW_DVBCORE_FAILURE;
    }

    //DVBCoreSPDeInit();
    DVBCoreTunerDeInit(dc->mTunerHandle);

    if(dc->DCListener)
    {
        free(dc->DCListener);
        dc->DCListener = NULL;
    }
    if(dc)
    {
        pthread_mutex_destroy(&dc->mStateMutex);
        free(dc);
        dc = NULL;
        pDVBCore = NULL;
    }

    printf("[Aw_DvbCore %s,%d] ! \n", __FUNCTION__, __LINE__);
    return AW_DVBCORE_SUCCESS;
}