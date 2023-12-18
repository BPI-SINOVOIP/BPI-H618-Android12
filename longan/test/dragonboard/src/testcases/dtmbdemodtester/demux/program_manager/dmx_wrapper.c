/**@file
*    @brief		The file of DMX module
*    @author	xhw
*    @date		2015-10-10
*    @version		1.0.0
*    @note		Copyright(C) Allwinner Corporation. All rights reserved.
*/
#define LOG_NDEBUG 0
#define LOG_TAG "demux_wrapper"
#include <DVBCoreLog.h>
#include <stdlib.h>
#include <dtv_base.h>
#include <dmx_wrapper.h>

#if defined(USE_AW_TSDEMUX)
#include <tsdemux.h>
#else
#include <tsdemux_vs.h>
#endif

#define USE_PACKET_QUEUE   1

#if USE_PACKET_QUEUE //add for epg search
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

typedef struct PacketNodeS
{
   int mPacketSize;
   unsigned char *pPacketData;
   struct PacketNodeS  *next;
}PacketNode;

typedef struct
{
    int mQueueSize;
    PacketNode *front;
    PacketNode *rear;
    pthread_mutex_t mQueueMutex;
}PacketQueue;

typedef struct
{
    int            mStartFlag;
    sem_t          mDmxSem;
    pthread_t      mProcessThreadId;
    PacketQueue    mPacketQueue;
    TSParser       *pTSParser;
}Dmx_Wrapper;

Dmx_Wrapper dmxWrapper;

PacketNode* CreateNode(int mSize)
{
    PacketNode *pNode = calloc(1, sizeof(PacketNode));
    if(NULL == pNode)
        return NULL;

    pNode->pPacketData = (unsigned char*)calloc(1, mSize * sizeof(unsigned char));
    if(NULL == pNode->pPacketData)
    {
        free(pNode);
        return NULL;
    }

    pNode->mPacketSize = mSize;
    pNode->next = NULL;

    return pNode;
}

void DeleteNode(PacketNode* pNode)
{
    if(NULL == pNode)
        return;

    if(pNode->pPacketData)
    {
         free(pNode->pPacketData);
         pNode->pPacketData = NULL;
    }
    free(pNode);
    return;
}

int PacketQueueInit(PacketQueue *pQueue)
{
    if(NULL == pQueue)
        return -1;

    pQueue->front = NULL;
    pQueue->rear  = NULL;
    pQueue->mQueueSize = 0;
    pthread_mutex_init(&pQueue->mQueueMutex, NULL);
    DVBCORE_LOGD("packet queue init finish!\n");

    return 0;
}

int PacketQueueEmpty(PacketQueue *pQueue)
{
    return (pQueue->mQueueSize == 0 && pQueue->front == NULL && pQueue->rear == NULL);
}

int PacketQueuePush(PacketQueue *pQueue, PacketNode* pNode)
{
    if(pQueue == NULL || pNode == NULL)
        return -1;

    pthread_mutex_lock(&pQueue->mQueueMutex);
    if(PacketQueueEmpty(pQueue))
    {
        pQueue->front = pNode;
        pQueue->rear  = pNode;
    }
    else
    {
        pQueue->rear->next = pNode;
        pQueue->rear = pNode;
    }
    pQueue->mQueueSize++;
    pthread_mutex_unlock(&pQueue->mQueueMutex);
    //DVBCORE_LOGD("push %p finish, packetSize %d,  queue  size %d !", pNode, pNode->mPacketSize, pQueue->mQueueSize);

    return 0;
}

PacketNode* PacketQueuePop(PacketQueue *pQueue)
{
    if(pQueue == NULL  || PacketQueueEmpty(pQueue))
        return NULL;

    PacketNode *pHead = NULL;

    pthread_mutex_lock(&pQueue->mQueueMutex);
    pHead         = pQueue->front;
    pQueue->front = pHead->next;
    pQueue->mQueueSize--;

    if(pQueue->mQueueSize == 0)
        pQueue->rear = NULL;
    pthread_mutex_unlock(&pQueue->mQueueMutex);
    //DVBCORE_LOGD("pop %p , packetSize %d, queue size %d !", pHead, pHead->mPacketSize, pQueue->mQueueSize);

    return pHead;
}

void PacketQueueDestroy(PacketQueue *pQueue)
{
    if(NULL == pQueue)
        return;

    PacketNode *pHead = NULL;
    while(!PacketQueueEmpty(pQueue))
    {
        pHead = PacketQueuePop(pQueue);
        DeleteNode(pHead);
    }

    pthread_mutex_destroy(&pQueue->mQueueMutex);
    DVBCORE_LOGD("packet queue destroy finish!\n");

    return;
}

static void* dataProcessThread(void *arg)
{
    int ret = 0;
    int len = 0;
    int offset = 0;

    PacketNode  *pNode = NULL;
    Dmx_Wrapper *pDmx = (Dmx_Wrapper*)arg;

    sem_wait(&pDmx->mDmxSem);

    while(pDmx->mStartFlag)
    {
        offset = 0;
        pNode = PacketQueuePop(&pDmx->mPacketQueue);

        if(NULL == pNode)
        {
            usleep(1 * 1000);
            continue;
        }
        else
        {
            len = pNode->mPacketSize;
        }

        if(EIT_FILTER_TYPE != DMX_FILTER_TYPE_SECTION)
        {
            while(pNode->pPacketData && len > 0)
            {
                unsigned char *pBuf = pNode->pPacketData + offset;
                ret = feedTSPacket(pDmx->pTSParser, pBuf, TS_PACKET_SIZE);
                len -= TS_PACKET_SIZE;
                if(ret < 0 || !pDmx->mStartFlag)
                {
                    DVBCORE_LOGE("parse eit table error or start flag %d !", pDmx->mStartFlag);
                    break;
                }
                offset += TS_PACKET_SIZE;
            }
        }
        else
        {
            if(!pDmx->mStartFlag)
            {
                DVBCORE_LOGE("parse eit exit flag set!\n");
                break;
            }

            parseSectionPacket(pDmx->pTSParser, EIT_PID, pNode->pPacketData, len);
        }

        DeleteNode(pNode);
    }

    DVBCORE_LOGD("exit date process thread !\n");
    pthread_exit(NULL);
    return 0;
}
#endif

int PatRequestBuffer(void * arg, void *cookie)
{  
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;
	

	info->buffer = mTSParser->SectionBufer[PAT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	
	if(mTSParser->bIsEndPatSection == 1) // has parsed pat section success
	{
		pbuffer->buf	 =   NULL;
		pbuffer->buf_size =   0;
	}
	else if((mTSParser->SectionBufer[PAT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[PAT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;//mTSParser->SectionBufer[PAT_BUF_IDX];
		pbuffer->buf_size =  info->buf_size;//SECTION_BUF_SIZE;
	}
	return 0;	
}

int PmtRequestBuffer(void * arg, void *cookie)
{  
	
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;

	info->buffer = mTSParser->SectionBufer[PMT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	
	if(mTSParser->bIsEndPmtSection == 1) // has parsed pat section success
	{
		pbuffer->buf	 =   NULL;
		pbuffer->buf_size =   0;
	}
	else if((mTSParser->SectionBufer[PMT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[PMT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;//mTSParser->SectionBufer[PMT_BUF_IDX];
		pbuffer->buf_size =  info->buf_size;//SECTION_BUF_SIZE;
	}
	return 0;	
}

int SdtRequestBuffer(void * arg, void *cookie)
{  
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;
	info->buffer = mTSParser->SectionBufer[SDT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	if(mTSParser->bIsEndSdtSection == 1) // has parsed pat section success
	{
		pbuffer->buf	 =   NULL;
		pbuffer->buf_size =   0;
	}
	else if((mTSParser->SectionBufer[SDT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[SDT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;//mTSParser->SectionBufer[SDT_BUF_IDX];
		pbuffer->buf_size =  info->buf_size;//SECTION_BUF_SIZE;
	}
	return 0;		
}

int NitRequestBuffer(void * arg, void *cookie)
{  
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;
	info->buffer = mTSParser->SectionBufer[NIT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	
	if(mTSParser->bIsEndNitSection == 1) // has parsed pat section success
	{
		pbuffer->buf	 =   NULL;
		pbuffer->buf_size =   0;
	}
	else if((mTSParser->SectionBufer[NIT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[NIT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;//mTSParser->SectionBufer[NIT_BUF_IDX];
		pbuffer->buf_size =  info->buf_size;//SECTION_BUF_SIZE;
	}
	return 0;	

}

int TdtRequestBuffer(void * arg, void *cookie)
{  
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;
	info->buffer = mTSParser->SectionBufer[TDT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	
	if((mTSParser->SectionBufer[TDT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[TDT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;
		pbuffer->buf_size =  info->buf_size;
	}
	return 0;		
}

int EitRequestBuffer(void * arg, void *cookie)
{  
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;
	info->buffer = mTSParser->SectionBufer[EIT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	
	if((mTSParser->SectionBufer[EIT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[EIT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;
		pbuffer->buf_size =  info->buf_size;
	}
	return 0;		
}

int CatRequestBuffer(void * arg, void *cookie)
{  
    filter_info *info = (filter_info*)cookie;
    md_buf_t *pbuffer = ( md_buf_t*) arg;
	TSParser* mTSParser = info->mTSParser;
	info->buffer = mTSParser->SectionBufer[CAT_BUF_IDX];
	info->buf_size = SECTION_BUF_SIZE;
	
	if(mTSParser->bIsEndCatSection == 1) // has parsed pat section success
	{
		pbuffer->buf	 =   NULL;
		pbuffer->buf_size =   0;
	}
	else if((mTSParser->SectionBufer[CAT_BUF_IDX] == NULL)
			||(mTSParser->bIsBufferFull[CAT_BUF_IDX]==1))
	{
		pbuffer->buf	  =   NULL;
		pbuffer->buf_size =   0;;
	}
	else
	{
		pbuffer->buf	 =   info->buffer;
		pbuffer->buf_size =  info->buf_size;
	}
	return 0;		
}

int PatUpdateData(void * arg, void *cookie)             
{ 
    filter_info *info = (filter_info*)cookie;	
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	unsigned int len = *((unsigned int *)arg);	
	mTSParser->bIsBufferFull[PAT_BUF_IDX] = 1;
	
	while((len > 0) && (mTSParser->bIsEndPatSection ==0))
	{
		unsigned char *tmpBuf = info->buffer + offset;
		ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
		len -= TS_PACKET_SIZE;
		if(ret < 0)
		{
			break;			
		}
		offset += TS_PACKET_SIZE;
	}
	if(ret < 0)
	{
		mTSParser->bIsBufferFull[PAT_BUF_IDX] = 0;
		return -1;
	}
	else
	{
		mTSParser->bIsBufferFull[PAT_BUF_IDX] = 0;
    	return 0;
	}
}

int PmtUpdateData(void * arg, void *cookie)             
{ 
	
    filter_info *info = (filter_info*)cookie;
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	int len = *((int *)arg);
	mTSParser->bIsBufferFull[PMT_BUF_IDX] = 1;
	int i = 0;

	while((len > 0) && (mTSParser->bIsEndPmtSection ==0))
	{
		unsigned char *tmpBuf = info->buffer + offset;
		ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
		len -= TS_PACKET_SIZE;
		if(ret < 0)
		{
			break;			
		}
		offset += TS_PACKET_SIZE;
	}
	if(ret < 0)
	{
		mTSParser->bIsBufferFull[PMT_BUF_IDX] = 0;
		return -1;
	}
	else
	{
		mTSParser->bIsBufferFull[PMT_BUF_IDX] = 0;
    	return 0;
	}

}

int SdtUpdateData(void * arg, void *cookie)     
{ 
    filter_info *info = (filter_info*)cookie;
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	int len = *((int *)arg);
	mTSParser->bIsBufferFull[SDT_BUF_IDX] = 1;
	
	while((len > 0) && (mTSParser->bIsEndSdtSection ==0))
	{
		unsigned char *tmpBuf = info->buffer + offset;
		ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
		len -= TS_PACKET_SIZE;
		if(ret < 0)
		{
			break;			
		}
		offset += TS_PACKET_SIZE;
	}
	if(ret < 0)
	{
		mTSParser->bIsBufferFull[SDT_BUF_IDX] = 0;
		return -1;
	}
	else
	{
		mTSParser->bIsBufferFull[SDT_BUF_IDX] = 0;
    	return 0;
	}

}

int NitUpdateData(void * arg, void *cookie)
{ 
    filter_info *info = (filter_info*)cookie;
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	int len = *((int *)arg);
	mTSParser->bIsBufferFull[NIT_BUF_IDX] = 1;
	
	while((len > 0) && (mTSParser->bIsEndNitSection ==0))
	{
		unsigned char *tmpBuf = info->buffer + offset;
		ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
		len -= TS_PACKET_SIZE;
		if(ret < 0)
		{
			break;			
		}
		offset += TS_PACKET_SIZE;
	}
	if(ret < 0)
	{
		mTSParser->bIsBufferFull[NIT_BUF_IDX] = 0;
		return -1;
	}
	else
	{
		mTSParser->bIsBufferFull[NIT_BUF_IDX] = 0;
    	return 0;
	}

}

int TdtUpdateData(void * arg, void *cookie)             
{ 
    filter_info *info = (filter_info*)cookie;
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	int len = *((int *)arg);
	mTSParser->bIsBufferFull[TDT_BUF_IDX] = 1;

	#ifdef DMX_DUBUG
	DVBCORE_LOGD("Tdt-len: %d %x", len,info->buffer);
	DVBCORE_BUF_DUMP(info->buffer,188);
	#endif
	
	while(len > 0)
	{
		unsigned char* tmpBuf = info->buffer + offset;
		ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
		len -= TS_PACKET_SIZE;
		if(ret < 0)
		{
			break;			
		}
		offset += TS_PACKET_SIZE;
	}
	if(ret < 0)
	{
		mTSParser->bIsBufferFull[TDT_BUF_IDX] = 0;
		return -1;
	}
	else
	{
		mTSParser->bIsBufferFull[TDT_BUF_IDX] = 0;
    	return 0;
	}

}

int EitUpdateData(void * arg, void *cookie)
{ 
    filter_info *info = (filter_info*)cookie;
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	int len = *((int *)arg);
	mTSParser->bIsBufferFull[EIT_BUF_IDX] = 1;
    #ifdef DMX_DUBUG
	DVBCORE_LOGD("eit-len: %d, packet num: %d", len, len/188);
	DVBCORE_BUF_DUMP(info->buffer, 188);
    #endif

#if !USE_PACKET_QUEUE
    if(EIT_FILTER_TYPE != DMX_FILTER_TYPE_SECTION)
    {
        while(len > 0)
        {
            unsigned char *tmpBuf = info->buffer + offset;
            ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
            len -= TS_PACKET_SIZE;
            if(ret < 0)
            {
                DVBCORE_LOGE("parse eit ts packet error.\n");
                break;
            }
            offset += TS_PACKET_SIZE;
        }
    }
    else
    {
        ret = parseSectionPacket(mTSParser, EIT_PID, info->buffer, len);
        if(ret < 0)
        {
            DVBCORE_LOGE("parse eit section packet error.\n");
        }
    }
#else
    PacketNode* pNode = NULL;
    if(len > 0 && info->buffer)
    {
        pNode = CreateNode(len);
        if(pNode)
        {
            memcpy(pNode->pPacketData, info->buffer, len);
            ret = PacketQueuePush(&dmxWrapper.mPacketQueue, pNode);
        }
        else
            ret = -1;
    }
#endif

	if(ret < 0)
	{
		mTSParser->bIsBufferFull[EIT_BUF_IDX] = 0;
		return ret;
	}
	else
	{
		mTSParser->bIsBufferFull[EIT_BUF_IDX] = 0;
    	return 0;
	}

}

int CatUpdateData(void * arg, void *cookie)   
{ 
    filter_info *info = (filter_info*)cookie;
    TSParser* mTSParser = info->mTSParser;
	int ret = 0;
	int offset = 0;
	int len = *((int *)arg);
	mTSParser->bIsBufferFull[CAT_BUF_IDX] = 1;
	
	while((len > 0) && (mTSParser->bIsEndCatSection ==0))
	{
		unsigned char *tmpBuf = info->buffer + offset;
		ret = feedTSPacket(mTSParser, tmpBuf, TS_PACKET_SIZE);
		len -= TS_PACKET_SIZE;
		if(ret < 0)
		{
			break;			
		}
		offset += TS_PACKET_SIZE;
	}
	
	if(ret < 0)
	{
		mTSParser->bIsBufferFull[CAT_BUF_IDX] = 0;
		return -1;
	}
	else
	{
		mTSParser->bIsBufferFull[CAT_BUF_IDX] = 0;
    	return 0;
	}

}

int DVBCoreDemuxOpenFilter(int *demux_handle, filter_info *filter)
{
    int  result;
	demux_filter_param_t param;
	param.cookie = filter;
#if defined(USE_AW_TSDEMUX)
	param.filter_type = DMX_FILTER_TYPE_TS;
    param.tsf_port = 0;
    param.ts_input_port = 0;
#else
    int32_t pid_arry[] = {EIT_PID};
    uint8_t tableid[EIT_SCHEDULE_TABLE_ID_NUM] = {0x50, 0x51};
    param.filter_type = EIT_FILTER_TYPE;
    if(EIT_FILTER_TYPE == DMX_FILTER_TYPE_SECTION)
    {
        param.pids = &filter->pid;
        param.pid_num = 1;
        param.tableids = tableid;
        param.tableid_num = EIT_SCHEDULE_TABLE_ID_NUM;
    }
    else if(EIT_FILTER_TYPE == DMX_FILTER_TYPE_TS)
    {
        param.pids = pid_arry;
        param.pid_num = sizeof(pid_arry)/sizeof(pid_arry[0]);
    }
#endif
    
    switch(filter->pid)
    {
       case  PAT_PID:
             {
                param.request_buffer_cb = PatRequestBuffer;
                param.update_data_cb    = PatUpdateData;
                break;
             }
       case  CAT_PID:     
             {  
				param.request_buffer_cb = CatRequestBuffer;		 
				param.update_data_cb	= CatUpdateData;
				break;
             }
       case  TSDT_PID:
             {
                break;
             }
       case  NIT_PID:
             {
				param.request_buffer_cb = NitRequestBuffer;         
				param.update_data_cb	= NitUpdateData;
                break;
             }
       case  SDT_BAT_PID:
             {
                param.request_buffer_cb = SdtRequestBuffer;         
                param.update_data_cb  	= SdtUpdateData;
                break;
             }
       case  EIT_PID:
             {
                param.request_buffer_cb = EitRequestBuffer;         
                param.update_data_cb  	= EitUpdateData;
                break;
             }
       case  TDT_TOT_PID:
             {
                param.request_buffer_cb = TdtRequestBuffer;         
                param.update_data_cb    = TdtUpdateData;
                break;
             }
       case  DIT_PID:
             {
                break;
             }
       case  SIT_PID:
             {
                break;
             }
       default:
           {			   
               param.request_buffer_cb = PmtRequestBuffer;         
               param.update_data_cb    = PmtUpdateData;
               break;
           }
    }

#if USE_PACKET_QUEUE
    memset(&dmxWrapper, 0x00, sizeof(dmxWrapper));
    dmxWrapper.pTSParser = filter->mTSParser;
    sem_init(&dmxWrapper.mDmxSem, 0, 0);

    if(pthread_create(&dmxWrapper.mProcessThreadId, NULL, dataProcessThread, &dmxWrapper) == 0)
    {
        PacketQueueInit(&dmxWrapper.mPacketQueue);
        dmxWrapper.mStartFlag = 1;
        sem_post(&dmxWrapper.mDmxSem);
        DVBCORE_LOGD("create date process thread %lu finish!", dmxWrapper.mProcessThreadId);
    }
#endif

#if defined(USE_AW_TSDEMUX)
	result = ts_demux_open_filter(demux_handle, filter->pid, &param);
#else
    DVBCORE_LOGD("open filter type %d !", param.filter_type);
    result = ts_demux_vs_open_filter(demux_handle, &param);
#endif

	if(result != 0) 
	{
		DVBCORE_LOGE("open filter fail!\n\n");
		*demux_handle = -1;
		return -1;
	}
	else
    {
    	return 0;
	}
}

int DVBCoreDemuxCloseFilter(int *demux_handle, int pid, int nChanId)
{
    int result;
    (void)nChanId;

	if(demux_handle == NULL)
	{
		DVBCORE_LOGW("demux_handle is NULL!\n");
		return -1;
	}

#if defined(USE_AW_TSDEMUX)
    result = ts_demux_close_filter(demux_handle, pid);
#else
    result = ts_demux_vs_close_filter(demux_handle, EIT_FILTER_TYPE);
#endif

#if USE_PACKET_QUEUE
    if(dmxWrapper.mProcessThreadId)
    {
        dmxWrapper.mStartFlag = 0;
        pthread_join(dmxWrapper.mProcessThreadId, NULL);
        PacketQueueDestroy(&dmxWrapper.mPacketQueue);
        sem_destroy(&dmxWrapper.mDmxSem);

        DVBCORE_LOGD("wait data process thread %lu finish !", dmxWrapper.mProcessThreadId);
    }
#endif

    if(result < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


