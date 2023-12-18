/**@file
*    @brief		The file of Program Manager module
*    @author	xhw
*    @date		2015-10-12
*    @version	1.0.0
*    @note		Copyright(C) Allwinner Corporation. All rights reserved.
*/
/**************************************************************************/

#define LOG_TAG "program_manager"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include "DVBCoreEpg.h"
#include "ProgramManager.h"
#include "DVBCoreMemory.h"
#include "dmx_wrapper.h"
#include "DVBCoreLog.h"

#if defined(USE_AW_TSDEMUX)
#include <tsdemux.h>
#else
#include <tsdemux_vs.h>
#endif

//#include <streamProducer.h>

#define WAIT_SDT_TIMEOUT    (5)  /*5 seconds*/
#define WAIT_NIT_TIMEOUT    (3)
#define WAIT_PAT_TIMEOUT    (5)
#define WAIT_PMT_TIMEOUT    (3)
#define WAIT_EIT_TIMEOUT    (30)

static int64_t lGetNowUS(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (long long)tv.tv_sec * 1000000ll + tv.tv_usec;
}

static void EpgManagerReset(void* arg)
{
    int i ;
	EPGContext* pEpgContext;
	pEpgContext = (EPGContext*) arg;

    for(i = 0;i < EIT_SCHEDULE_TABLE_ID_NUM; i++)
    {
        pEpgContext->mTSParser->schdl_count[i] = 0;
        pEpgContext->mTSParser->schdl_parse_end[i] = 0;
		pEpgContext->mTSParser->schdl_eit_version_number[i] = (unsigned char)-1;
        if(pEpgContext->mTSParser->pProgramEvent[i] != NULL)
        {
            FreeProgramEventMemory(pEpgContext->mTSParser->pProgramEvent[i]);
            pEpgContext->mTSParser->pProgramEvent[i] = NULL;
        }
    }
    if(pEpgContext->mTSParser->pPresentEvent != NULL)
    {
        DVBCORE_LOGD(" free pEpgContext->mTSParser->pPresentEvent !");
        FreeProgramEventMemory(pEpgContext->mTSParser->pPresentEvent);
        pEpgContext->mTSParser->pPresentEvent = NULL;
    }
    if(pEpgContext->mTSParser->pFollowEvent != NULL)
    {
        DVBCORE_LOGD(" free pEpgContext->mTSParser->pFollowEvent !");
        FreeProgramEventMemory(pEpgContext->mTSParser->pFollowEvent);
        pEpgContext->mTSParser->pFollowEvent = NULL;
    }

    memset(&pEpgContext->mTSParser->schdl_sec_mask, 0, EIT_SCHEDULE_TABLE_ID_NUM*32*sizeof(SCH_SEC_MASK));
    memset(&pEpgContext->mTSParser->pf_sec_mask, 0, sizeof(SCH_SEC_MASK));

	pEpgContext->mTSParser->pf_eit_version_number = (unsigned char)-1;
    pEpgContext->mTSParser->event_total_count = 0;
    pEpgContext->mTSParser->pf_count = 0;
    pEpgContext->mTSParser->pf_parse_end = 0;//reset for next program_num parse.
    pEpgContext->mTSParser->prog_num = 0;
    pEpgContext->mTSParser->bIsEndEitSection = 0;

    return;
}

int PMCompSetCallback(void *p, PMCallbackInternal callback, void* pUserData)
{
    PMContext* pPMContext;
    pPMContext = (PMContext*)p;
    pPMContext->callback  = callback;
    pPMContext->pUserData = pUserData;
    return 0;
}

static void PMContextReset(PMContext* pPMContext)
{
	DVBCORE_LOGD("PM Context Reset");
	if(pPMContext)
	{
		pPMContext->eStatus = DVBCORE_PM_INITIALIZED;
		pPMContext->mErrno = DVBCORE_INVALID;
		pPMContext->nHasSearchedNum = 0;
		pPMContext->bTimeMonitorStopFlag = 0;
		//pPMContext->bEpgMonitorStopFlag = 0;
		pPMContext->bEmmGetStopFlag = 0;
		pPMContext->end = 0;
		pPMContext->mErrno = 0;
	}
	else
	{
		DVBCORE_LOGW("maybe error, pPMContext should not be NULL here!");
		return;
	}

	if(pPMContext->mTSParser)
	{
		pPMContext->mTSParser->type = EPG_TYPE_NONE;
		pPMContext->mTSParser->mProgramCount = 0;

		pPMContext->mTSParser->pat_version_number = (unsigned char)-1;
		pPMContext->mTSParser->pmt_version_number = (unsigned char)-1;
		pPMContext->mTSParser->sdt_version_number = (unsigned char)-1;
		pPMContext->mTSParser->pf_eit_version_number = (unsigned char)-1;
		pPMContext->mTSParser->nit_version_number = (unsigned char)-1;
		pPMContext->mTSParser->cat_version_number = (unsigned char)-1;

		pPMContext->mTSParser->bIsEndSdtSection = 0;
		pPMContext->mTSParser->bIsEndPatSection = 0;
		pPMContext->mTSParser->bIsEndPmtSection = 0;
		pPMContext->mTSParser->bIsEndNitSection = 0;
		pPMContext->mTSParser->bIsEndEitSection = 0;
		pPMContext->mTSParser->bIsEndCatSection = 0;
	}

	if(pPMContext->mTSParser->pProgram)
	{
		FreeProgramMemory(pPMContext->mTSParser->pProgram);
		pPMContext->mTSParser->pProgram = NULL;
	}
}

PMContext* CreateProgramManager(void)
{
	DVBCORE_LOGD("Create Program Manager.");
	#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
	DVBCoreMeroryDebugInit();
	#endif
	PMContext* pPMContext = (PMContext*)DVBCoreMalloc(sizeof(PMContext));
    if(pPMContext == NULL)
    {
		DVBCORE_LOGE("malloc failed.");
		return NULL;
    }
    memset(pPMContext, 0, sizeof(PMContext));
	pPMContext->mTSParser = TSParserOpen();

	if(pPMContext->mTSParser == NULL)
    {
		DVBCORE_LOGE("malloc failed.");
		return NULL;
    }

	pPMContext->mTSParser->MallocBufer =
		(unsigned char*)DVBCoreMalloc(2*sizeof(unsigned char)*SECTION_BUF_SIZE);
    if(pPMContext->mTSParser->MallocBufer == NULL)
    {
		DVBCORE_LOGE("malloc failed.");
        return NULL;
    }

	pPMContext->mTSParser->SectionBufer[0] =
		(unsigned char *)pPMContext->mTSParser->MallocBufer;
	pPMContext->mTSParser->SectionBufer[1] =
		(unsigned char *)(pPMContext->mTSParser->SectionBufer[0] + SECTION_BUF_SIZE);
    memset(pPMContext->mTSParser->MallocBufer, 0, 2*sizeof(unsigned char)*SECTION_BUF_SIZE);
	TSParserInit(pPMContext->mTSParser);
    pthread_mutex_init(&pPMContext->statusLock, NULL);
    pthread_cond_init(&pPMContext->cond, NULL);
	pPMContext->eStatus = DVBCORE_PM_INITIALIZED;
	pPMContext->mErrno = DVBCORE_INVALID;
    return pPMContext;
}

EPGContext* CreateEpgManager(void)
{
	DVBCORE_LOGD("Create Epg Manager.");
	#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
	DVBCoreMeroryDebugInit();
	#endif
	EPGContext* pEPGContext = (EPGContext*)DVBCoreMalloc(sizeof(EPGContext));
    if(pEPGContext == NULL)
    {
		DVBCORE_LOGE("malloc failed.");
		return NULL;
    }
    memset(pEPGContext, 0, sizeof(EPGContext));
	pEPGContext->mTSParser = TSParserOpen();

	if(pEPGContext->mTSParser == NULL)
    {
		DVBCORE_LOGE("malloc failed.");
		return NULL;
    }

	pEPGContext->mTSParser->MallocBufer =
		(unsigned char*)DVBCoreMalloc(2*sizeof(unsigned char)*SECTION_BUF_SIZE);
    if(pEPGContext->mTSParser->MallocBufer == NULL)
    {
		DVBCORE_LOGE("malloc failed.");
        return NULL;
    }

	pEPGContext->mTSParser->SectionBufer[0] =
		(unsigned char *)pEPGContext->mTSParser->MallocBufer;
	pEPGContext->mTSParser->SectionBufer[1] =
		(unsigned char *)(pEPGContext->mTSParser->SectionBufer[0] + SECTION_BUF_SIZE);
    memset(pEPGContext->mTSParser->MallocBufer, 0, 2*sizeof(unsigned char)*SECTION_BUF_SIZE);
	TSParserInit(pEPGContext->mTSParser);
    pthread_mutex_init(&pEPGContext->statusLock, NULL);
    pthread_cond_init(&pEPGContext->cond, NULL);
    return pEPGContext;
}


int ProgramManagerGetStatus(PMContext* pPMContext)
{
    return pPMContext->eStatus;
}

int ProgramManagerReset(PMContext* pPMContext)
{
	if(pPMContext == NULL)
    {
        return -1;
    }
    else
    {
		PMContextReset(pPMContext);
		return 0;
    }
}

int ProgramManagerDestroy(PMContext* pPMContext)
{
	DVBCORE_LOGD("ProgramManagerDestroy");
	if( pPMContext == NULL)
    {
#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
		DVBCoreMeroryLeakDebugInfo();
		DVBCoreMeroryDebugClose();
#endif
        return 0;
    }
    else
    {
    	if(pPMContext->mTSParser)
    	{
			if(pPMContext->mTSParser->MallocBufer != NULL)
			{
				DVBCoreFree(pPMContext->mTSParser->MallocBufer);
				pPMContext->mTSParser->MallocBufer = NULL;
			}
			if(pPMContext->mTSParser->pProgram != NULL)
			{
				FreeProgramMemory(pPMContext->mTSParser->pProgram);
				pPMContext->mTSParser->pProgram = NULL;
			}
			TSParserClose(pPMContext->mTSParser);
			pPMContext->mTSParser = NULL;
    	}
		pthread_mutex_destroy(&pPMContext->statusLock);
		pthread_cond_destroy(&pPMContext->cond);
		DVBCoreFree(pPMContext);
#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
		DVBCoreMeroryLeakDebugInfo();
		DVBCoreMeroryDebugClose();
#endif
    	return 0;
    }
}

int EpgManagerDestroy(EPGContext* pEpgContext)
{
	DVBCORE_LOGD("EpgManagerDestroy");
	if( pEpgContext == NULL)
    {
#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
		DVBCoreMeroryLeakDebugInfo();
		DVBCoreMeroryDebugClose();
#endif
        return 0;
    }
    else
    {
    	if(pEpgContext->mTSParser)
    	{
    		int i = 0;
			if(pEpgContext->mTSParser->MallocBufer != NULL)
			{
				DVBCoreFree(pEpgContext->mTSParser->MallocBufer);
				pEpgContext->mTSParser->MallocBufer = NULL;
			}

			for(i = 0;i < EIT_SCHEDULE_TABLE_ID_NUM; i++)
			{
				if(pEpgContext->mTSParser->pProgramEvent[i] != NULL)
				{
					FreeProgramEventMemory(pEpgContext->mTSParser->pProgramEvent[i]);
					pEpgContext->mTSParser->pProgramEvent[i] = NULL;
				}
			}
			if(pEpgContext->mTSParser->pPresentEvent != NULL)
			{
				FreeProgramEventMemory(pEpgContext->mTSParser->pPresentEvent);
				pEpgContext->mTSParser->pPresentEvent = NULL;
			}
			if(pEpgContext->mTSParser->pFollowEvent != NULL)
			{
				FreeProgramEventMemory(pEpgContext->mTSParser->pFollowEvent);
				pEpgContext->mTSParser->pFollowEvent = NULL;
			}
			TSParserClose(pEpgContext->mTSParser);
			pEpgContext->mTSParser = NULL;
    	}
		pthread_mutex_destroy(&pEpgContext->statusLock);
		pthread_cond_destroy(&pEpgContext->cond);
		DVBCoreFree(pEpgContext);
#if DVBCORE_ENABLE_MEMORY_LEAK_DEBUG
		DVBCoreMeroryLeakDebugInfo();
		DVBCoreMeroryDebugClose();
#endif
    	return 0;
    }
}


static void* ProgramSearchThread(void* arg)
{
	PMContext* pPMContext;
	pPMContext = (PMContext*) arg;

	int i = 0;
	int j = 0;
	int	k = 0;
	int ret = 0;
	int nSdtChanId = -1;
	int nPatChanId = -1;
	int nPmtChanId = -1;
	int nNitChanId = -1;
	DVBCORE_LOGD("ProgramSearchThread begin.");
	pthread_mutex_lock(&pPMContext->statusLock);
	pPMContext->eStatus = DVBCORE_PM_SEARCHING;
	pthread_mutex_unlock(&pPMContext->statusLock);
	static filter_info pmt_info;
	//* search pat.
	static filter_info section_info;
	memset(&section_info, 0, sizeof(filter_info));
	section_info.mTSParser = pPMContext->mTSParser;

	struct timespec abstime;
	int retcode = 0;
	pthread_mutex_lock(&pPMContext->mTSParser->mutex); /*lock*/
	abstime.tv_sec = time(NULL) + WAIT_PAT_TIMEOUT; /* 5s */
	abstime.tv_nsec = 0;

	section_info.pid = PAT_PID;
	DVBCoreDemuxOpenFilter(pPMContext->demux_handle, &section_info);
	pPMContext->mTSParser->bIsEndPatSection = 0;
	while(pPMContext->mTSParser->bIsEndPatSection == 0)
	{
		retcode = pthread_cond_timedwait(&pPMContext->mTSParser->cond,
			&pPMContext->mTSParser->mutex, &abstime);
		if(retcode == ETIMEDOUT)
		{
			DVBCORE_LOGE("search PAT timeout.");
			pPMContext->mErrno = DVBCORE_SEARCH_FAIL;
			break;
		}
	}
	DVBCoreDemuxCloseFilter(pPMContext->demux_handle, PAT_PID, nSdtChanId);
	pthread_mutex_unlock(&pPMContext->mTSParser->mutex); /*unlock*/

	if(pPMContext->mTSParser->bIsEndPatSection == 1)
	{
		pthread_mutex_lock(&pPMContext->statusLock);
		pPMContext->eStatus = DVBCORE_PM_PAT_SEARCHED;
		pthread_mutex_unlock(&pPMContext->statusLock);

		PROGRAM * p = pPMContext->mTSParser->pProgram;
		while(p)
		{
			if (p->uProgramNumber != 0)
			{
				DVBCORE_LOGD("program number:%d, pmt pid:%d",p->uProgramNumber, p->uPMT_PID);
				p = p->next;
			}
			else
			{
				p = p->next;
				continue;
			}
		}
		//* search pmt.
		p = pPMContext->mTSParser->pProgram;
		DVBCORE_LOGD("pPMContext->nHasSearchedNum: %d", pPMContext->nHasSearchedNum);
		while(p)
		{
			if (p->uProgramNumber != 0)
			{
				DVBCORE_LOGD("pmt pid:%d",p->uPMT_PID);
				memset(&pmt_info, 0, sizeof(filter_info));
				pmt_info.pid = p->uPMT_PID;
				pmt_info.mTSParser = pPMContext->mTSParser;
				pPMContext->mTSParser->bIsEndPmtSection = 0;
			}
			else
			{
				p = p->next;
				continue;
			}

			DVBCORE_LOGD("search pmt pid %d, open filter.",p->uPMT_PID);
			pthread_mutex_lock(&pPMContext->mTSParser->mutex); /*lock*/
			abstime.tv_sec = time(NULL) + WAIT_PMT_TIMEOUT; /* 3s */
			abstime.tv_nsec = 0;
			DVBCoreDemuxOpenFilter(pPMContext->demux_handle, &pmt_info);
			DVBCORE_LOGD("DVBCoreDemuxOpenFilter end.");
			retcode = 0;
			while(pPMContext->mTSParser->bIsEndPmtSection == 0)
			{
				retcode = pthread_cond_timedwait(&pPMContext->mTSParser->cond,
					&pPMContext->mTSParser->mutex, &abstime);
				if(retcode == ETIMEDOUT)
				{
					DVBCORE_LOGE("search PMT timeout.");
					pPMContext->mErrno = DVBCORE_SEARCH_FAIL;
					break;
				}
			}
			DVBCoreDemuxCloseFilter(pPMContext->demux_handle, p->uPMT_PID, nPmtChanId);
			DVBCORE_LOGD("search pmt pid %d, close filter.",p->uPMT_PID);
			pthread_mutex_unlock(&pPMContext->mTSParser->mutex); /*unlock*/

			if(pPMContext->mTSParser->bIsEndPmtSection == 1)
			{
				PROGRAM * p = pPMContext->mTSParser->pProgram;
				while(p)
				{
					if (p->uProgramNumber != 0)
					{
						DVBCORE_LOGD("v:%d a:%d a1:%d pcr:%d pmt:%d name:%s pn:%d\n",p->uVideoPID,
							p->uAudioPID[0],p->uAudioPID[1],p->uPCR_PID,
							p->uPMT_PID,p->pucProgramName,p->uProgramNumber);
						p = p->next;
					}
					else
					{
						p = p->next;
						continue;
					}
				}
				pPMContext->nHasSearchedNum++;
			}
			else
			{
				DVBCORE_LOGE("Search uPMT_PID(%d) fail!", p->uPMT_PID);
			}
			p = p->next;
		}
		if(pPMContext->nHasSearchedNum == 0)
		{
			DVBCORE_LOGE("Search All PMT fail!");
			goto _pmSearchExit;
		}
		else
		{
			pthread_mutex_lock(&pPMContext->statusLock);
			pPMContext->eStatus = DVBCORE_PM_PMT_SEARCHED;
			pthread_mutex_unlock(&pPMContext->statusLock);
		}
	}
	else
	{
		DVBCORE_LOGE("Search PAT fail,Need Search Again!");
		goto _pmSearchExit;
	}

	//* search sdt.
	pthread_mutex_lock(&pPMContext->mTSParser->mutex); /*lock*/
	abstime.tv_sec = time(NULL) + WAIT_SDT_TIMEOUT; /* 5s */
	abstime.tv_nsec = 0;
	section_info.pid = SDT_BAT_PID;
	DVBCoreDemuxOpenFilter(pPMContext->demux_handle, &section_info);
	retcode = 0;
	pPMContext->mTSParser->bIsEndSdtSection = 0;
	while(pPMContext->mTSParser->bIsEndSdtSection == 0)
	{
		retcode = pthread_cond_timedwait(&pPMContext->mTSParser->cond,
			&pPMContext->mTSParser->mutex, &abstime);
		if(retcode == ETIMEDOUT)
		{
			DVBCORE_LOGE("search SDT timeout.");
			break;
		}
	}
	DVBCoreDemuxCloseFilter(pPMContext->demux_handle, SDT_BAT_PID, nSdtChanId);
	pthread_mutex_unlock(&pPMContext->mTSParser->mutex); /*unlock*/

	//* search nit.
	if(pPMContext->mTSParser->bIsEndPatSection || pPMContext->mTSParser->bIsEndSdtSection)
	{
		pthread_mutex_lock(&pPMContext->mTSParser->mutex); /*lock*/
		abstime.tv_sec = time(NULL) + WAIT_NIT_TIMEOUT; /* 3s */
		abstime.tv_nsec = 0;
		section_info.pid = NIT_PID;
		DVBCoreDemuxOpenFilter(pPMContext->demux_handle, &section_info);
		pPMContext->mTSParser->bIsEndNitSection = 0;
		retcode = 0;
		while(pPMContext->mTSParser->bIsEndNitSection == 0)
		{
			retcode = pthread_cond_timedwait(&pPMContext->mTSParser->cond,
				&pPMContext->mTSParser->mutex, &abstime);
			if(retcode == ETIMEDOUT)
			{
				DVBCORE_LOGE("search NIT timeout.");
				break;
			}
		}
		DVBCoreDemuxCloseFilter(pPMContext->demux_handle, NIT_PID, nSdtChanId);
		pthread_mutex_unlock(&pPMContext->mTSParser->mutex); /*unlock*/
	}
	else
	{
	  	printf("no pat,sdt, we don't know tsId, so search nit is not needed!");
	}

_pmSearchExit:
	DVBCORE_LOGD("ProgramSearchThread end.");
	pPMContext->end = 1;
	return NULL;
}


static void* TimeMonitorThread(void* arg)
{
	PMContext* pPMContext;
	pPMContext = (PMContext*) arg;

	int nTdtChanId = -1;
	int ret = 0;
	int kill_rc = 0;
	DVBCORE_LOGD("TimeMonitorThread start.");
	filter_info section_info;
	memset(&section_info, 0, sizeof(filter_info));
	section_info.pid = TDT_TOT_PID;
	section_info.mTSParser = pPMContext->mTSParser;
	DVBCoreDemuxOpenFilter(pPMContext->demux_handle, &section_info);

	while(1)
	{
		kill_rc = pthread_kill(pPMContext->timeMonitorThreadId, 0);

		if((kill_rc == ESRCH) || (kill_rc == EINVAL))
		{
			printf("TimeMonitorThread may have already exited!");
			break;
		}
		usleep(10);
	}
	DVBCORE_LOGD("TimeMonitorThread exit.");
	DVBCoreDemuxCloseFilter(pPMContext->demux_handle, TDT_TOT_PID, nTdtChanId);
	pPMContext->bTimeMonitorStopFlag = 1;
	pthread_exit(NULL);
	return (void *) 0;
}

static void* EpgMonitorThread(void* arg)
{
	EPGContext* pEPGContext;
	pEPGContext = (EPGContext*) arg;

	int nEitChanId = -1;
	int ret = 0;
	int kill_rc = 0;
    int i = 0;
    int bIsParseEitTimeout = 0;

	DVBCORE_LOGD("EpgMonitorThread begin");
	filter_info section_info;
	memset(&section_info, 0, sizeof(filter_info));

    pEPGContext->mEpgSearchEnd = 0;
    pEPGContext->mTSParser->bIsEndEitSection = 0;

	section_info.pid = EIT_PID;
	section_info.mTSParser = pEPGContext->mTSParser;
	DVBCoreDemuxOpenFilter(pEPGContext->demux_handle, &section_info);
	pEPGContext->bIsStarted = 1;
	while(1)
	{
#if 0
		kill_rc = pthread_kill(pEPGContext->epgMonitorThreadId, 0);
		if((kill_rc == ESRCH) || (kill_rc == EINVAL))
		{
			DVBCORE_LOGW("epgMonitorThreadId may have already exited, kill_rc: %d!",kill_rc);
			break;
		}
#endif
        for(i = 0 ; i < pEPGContext->mSearchCount ; i++)
        {
            if(!pEPGContext->bIsStarted)
            {
                DVBCORE_LOGD("EpgMonitorThread process stop!");
                break;
            }

            pEPGContext->mTSParser->prog_num = pEPGContext->mProgNum[i];
            if(pEPGContext->mTSParser->prog_num == 0)
                continue;
            else
                DVBCORE_LOGD("start parse service id %d EIT info !", pEPGContext->mTSParser->prog_num);

            int64_t time_start = lGetNowUS();
            while(pEPGContext->bIsStarted)
            {
                if((lGetNowUS() - time_start) > WAIT_EIT_TIMEOUT * 1000000 &&
                    !bIsParseEitTimeout)
                {
                    bIsParseEitTimeout = 1;
                    pEPGContext->mTSParser->bIsEndEitSection = 1;
                    DVBCORE_LOGW("parser service id %d EIT info timeout !", pEPGContext->mTSParser->prog_num);
                }

                if(pEPGContext->mTSParser->bIsEndEitSection && pEPGContext->bIsStarted)
                {
                    if(pEPGContext->callback_i)
                    {
                        if(bIsParseEitTimeout == 0 || pEPGContext->mTSParser->event_total_count > 0)
                            pEPGContext->callback_i(1, pEPGContext->mTSParser->event_total_count, (void*)&pEPGContext->mTSParser->prog_num);
                        else
                            pEPGContext->callback_i(0, 0, (void*)&pEPGContext->mTSParser->prog_num);
                    }

                    break;
                }
                usleep(10 * 1000);
	        }
            DVBCORE_LOGD("parser service id %d EIT info cost (%.2f) s !",pEPGContext->mTSParser->prog_num, (lGetNowUS() - time_start)/1000000.0);

            bIsParseEitTimeout = 0;
            EpgManagerReset(pEPGContext);
        }
        break;
    }

    pEPGContext->mEpgSearchEnd = 1;
    DVBCORE_LOGD("EpgMonitorThread end");

	DVBCoreDemuxCloseFilter(pEPGContext->demux_handle, EIT_PID, nEitChanId);
	pthread_exit(NULL);
	return (void *) NULL;
}

int ProgramSearchStart(PMContext* pPMContext)
{
	pthread_mutex_lock(&pPMContext->statusLock);
	pPMContext->eStatus = DVBCORE_PM_IDLE;
	if(pPMContext->SearchThreadId != 0)
	{
		DVBCORE_LOGE("invalid pm s tart operation!");
		pPMContext->mErrno = DVBCORE_INVALID_OPERATION;
		pthread_mutex_unlock(&pPMContext->statusLock);
		return -1;
	}

	if(pthread_create(&pPMContext->SearchThreadId, NULL, ProgramSearchThread, (void*)pPMContext) != 0)
	{
		DVBCORE_LOGE("pm search pthread create error!");
		pthread_mutex_unlock(&pPMContext->statusLock);
		return -1;
	}
	pthread_mutex_unlock(&pPMContext->statusLock);
	return 0;
}

int ProgramSearchStop(PMContext* pPMContext)
{
	pthread_mutex_lock(&pPMContext->statusLock);
	if(pPMContext->SearchThreadId == 0)
	{
		DVBCORE_LOGE("pm invalid stop operation!");
		pPMContext->SearchThreadId = 0;
		pPMContext->mErrno = DVBCORE_INVALID_OPERATION;
		pthread_mutex_unlock(&pPMContext->statusLock);
		return 0;
	}
	pthread_join(pPMContext->SearchThreadId, NULL);
	pPMContext->SearchThreadId = 0;
	pPMContext->eStatus = DVBCORE_PM_IDLE;
	pthread_mutex_unlock(&pPMContext->statusLock);
	return 0;
}

int EpgMonitorStart(EPGContext* pEPGContext)
{
	if(pEPGContext->epgMonitorThreadId != 0)
	{
		DVBCORE_LOGE("invalid epg start operation!");
		return -1;
	}

	if(pthread_create(&pEPGContext->epgMonitorThreadId, NULL, EpgMonitorThread, (void*)pEPGContext) != 0)
	{
		DVBCORE_LOGE("epg search thread create error!");
		return -1;
	}
	DVBCORE_LOGD("bIsStarted: %d",pEPGContext->bIsStarted);
	return 0;
}

int EpgMonitorStop(EPGContext* pEPGContext)
{
	DVBCORE_LOGD("EpgMonitorStop begin.");
	if(pEPGContext->epgMonitorThreadId == 0)
	{
		DVBCORE_LOGE("epg invalid stop operation!");
		pEPGContext->epgMonitorThreadId = 0;
		return 0;
	}
	DVBCORE_LOGD("EpgMonitorStop finish.");
	pEPGContext->bIsStarted = 0;
	pthread_join(pEPGContext->epgMonitorThreadId, NULL);
	return 0;
}

int TimeMonitorStart(PMContext* pPMContext)
{
	if(pPMContext->timeMonitorThreadId != 0 && pPMContext->bTimeMonitorStopFlag == 0)
	{
		return -1;
	}

	if(pthread_create(&pPMContext->timeMonitorThreadId, NULL, TimeMonitorThread, (void*)pPMContext) != 0)
	{
		return -1;
	}

	return 0;
}

int TimeMonitorStop(PMContext* pPMContext)
{
	if(pPMContext->timeMonitorThreadId == 0 || pPMContext->bTimeMonitorStopFlag == 1)
	{
		return 0;
	}

	pthread_join(pPMContext->timeMonitorThreadId, NULL);
	pPMContext->timeMonitorThreadId = 0;
	return 0;
}

static void* EMMGetThread(void* arg)
{
	PMContext* pPMContext;
	pPMContext = (PMContext*) arg;

	int nCatChanId = -1;
	int ret = 0;
	int kill_rc = 0;
	static filter_info section_info;
	memset(&section_info, 0, sizeof(filter_info));
	section_info.pid = CAT_PID;
	section_info.mTSParser = pPMContext->mTSParser;
	DVBCoreDemuxOpenFilter(pPMContext->demux_handle, &section_info);
	while(1)
	{
		usleep(10);
		kill_rc = pthread_kill(pPMContext->emmGetThreadId, 0);
		if((kill_rc == ESRCH) || (kill_rc == EINVAL))
		{
			printf("emmGetThreadId may have already exited!");
			break;
		}
	}
	DVBCoreDemuxCloseFilter(pPMContext->demux_handle, CAT_PID, nCatChanId);
	pPMContext->bEmmGetStopFlag = 1;
	pthread_exit(NULL);
	return (void *) 0;

}

int EMMGetStart(PMContext* pPMContext)
{
	if(pPMContext->emmGetThreadId != 0)
	{
		DVBCORE_LOGE("invalid EMM get operation!");
		pPMContext->mErrno = DVBCORE_INVALID_OPERATION;
		return -1;
	}

	if(pthread_create(&pPMContext->emmGetThreadId, NULL, EMMGetThread, (void*)pPMContext) != 0)
	{
		DVBCORE_LOGE("EMM get thread create error!");
		pPMContext->mErrno = DVBCORE_UNKNOWN_ERR;
		return -1;
	}
	return 0;
}

int EMMGetStop(PMContext* pPMContext)
{
	if(pPMContext->emmGetThreadId == 0)
	{
		DVBCORE_LOGE("invalid EMM stop operation!");
		pPMContext->emmGetThreadId = 0;
		pPMContext->mErrno = DVBCORE_INVALID_OPERATION;
		return 0;
	}
	pthread_join(pPMContext->emmGetThreadId, NULL);
	pPMContext->emmGetThreadId = 0;
	return 0;
}

