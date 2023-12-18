#define ZLOG_TAG "Awesome_PGS"

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <CdxList.h>
#include <CdxQueue.h>
#include <AwPool.h>
#include <cdx_log.h>

#include <CdxBitReader.h>
#include <dc_demod.h>
#include <ts_types.h>
#if defined(USE_AW_TSDEMUX)
#include <tsdemux.h>
#ifdef USE_TSC_DEV
#include <tsc.h>
#include <tsc_hal.h>
#endif
#else
#include "tsdemux_vs.h"
#endif

#include <ts_buffer.h>
#include <awesome_pgsearch.h>
#include <CharSetConvert.h>
#include <sys/time.h>
#define CRC32_SIZE 4

//#define PGS_DEBUG

enum
{
    PGS_STATE_INITINAL = 0,
    PGS_STATE_RUNNING,
    PGS_STATE_STOP,
    PGS_STATE_FINISHED,
    PGS_STATE_TIMEOUT,
    PGS_STATE_FAULT,
};

#define PGS_STATUS_ERROR (0x1 << 31)
#define PGS_STATUS_PAT_F (0x1)
#define PGS_STATUS_SDT_F (0x1 << 2)
#define PGS_STATUS_PMT_F (0x1 << 3)

//#define PGS_STATUS_INITINAL 0x0
#define PGS_STATUS_ALLDONE (PGS_STATUS_PAT_F|PGS_STATUS_SDT_F|PGS_STATUS_PMT_F)

#define TS_DESC_TAG_SERVICE 0x48
#define TS_DESC_TAG_MULTI_NAME 0x5D
#define TS_DESC_TAG_CA 0x09

#define PGS_TIMEOUT_5S 5000000

#define SECTION_LENTH_MAX 0x3FD
#define SDT_DESCRIPTOR_MAX (SECTION_LENTH_MAX - 8)

#define TS_PACKET_SIZE              188

#define SDT_SINGLE_SECTION_MAX (TS_PACKET_SIZE - 4 - 1 - 3)
#define SDT_SINGLE_DESCTIPTOR_TO_CRC_MAX (SDT_SINGLE_SECTION_MAX - 8)

#define PAT_SINGLE_SECTION_MAX SDT_SINGLE_SECTION_MAX

#define TS_SUBPACKET_MAX    (TS_PACKET_SIZE - 4)

#define TSC_LENGTH (1024 * TS_PACKET_SIZE)

#define PMT_SECTION_MAX_NUMBER      1

struct TsPackageS
{
    TSBufferT *rawbuf;
    uint8_t *ptr;
    uint16_t pid;
    CdxListNodeT node;
};

struct TsArrayS
{
    int data_came;
    CdxListT list;
};

struct TsPackagePoolS
{
    struct TsArrayS ts_arrs[8192];
};

struct PGSctxS
{
#ifdef USE_TSC_DEV
    ts_dev_t *tsc_hdr;
#endif
    DC_DemodT *demod;
    DvbStandardType stdType;
    uint32_t freq;
    int chan;
    CdxQueueT *buf_queue;
    AwPoolT *pool;

    uint32_t state;
    uint32_t status_flags;
    int64_t start_time;

    uint32_t program_cnt;
    uint32_t valid_program_cnt;
    uint32_t success_program_cnt;
    struct ProgramInfoS program_infos[16];
//    int program_imap_tab[16];

    struct PGS_ListenerS *listener;
    void *cookie;

    pthread_t fetch_thread;
    pthread_t parse_thread;

    struct TsPackagePoolS *tsp_pool;
};

struct SDTSubpacketS
{
    uint8_t  current_subpacket_cnt;
    uint32_t packet_sum;
    uint32_t packet_sum_left;
    uint32_t section_length;
    uint8_t  descriptor_buf[SDT_DESCRIPTOR_MAX];
    uint32_t descriptor_buf_ptr;
    uint32_t service_cnt;
    CdxListT service_list;
    uint32_t CRC_32;
};

typedef struct awesome_filter_info {
	uint8_t *buffer;
	int32_t buf_size;
    PGShandlerT *pgs;
    void* tsc_hdr;
} awesome_filter_info;


static inline uint16_t TS_GetPID(uint8_t *tsbuf)
{
    return ((tsbuf[1]&0x1f)<<8) + tsbuf[2];
}

static int64_t lGetNowUS(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (long long)tv.tv_sec * 1000000ll + tv.tv_usec;
}

static struct TsPackageS *newTsPackage(AwPoolT *pool, TSBufferT *buf, int offset)
{
    struct TsPackageS *tsp;

    tsp = Palloc(pool, sizeof(*tsp));

    tsp->rawbuf = TSBufferIncref(buf);
    tsp->ptr = TSBufferGetbufptr(buf) + offset;
    tsp->pid = TS_GetPID(tsp->ptr);
    //printf("new TS Package, off:'%d', pid:'%hu'", offset, tsp->pid);
    return tsp;
}

static int deleteTsPackage(AwPoolT *pool, struct TsPackageS *tsp)
{
    TSBufferDecref(tsp->rawbuf);
    Pfree(pool, tsp);
    return 0;
}

static int programNum2Index(PGShandlerT *pgs, int program_num)
{
	int i = 0;
	for (; i < pgs->program_cnt; i++)
	{
		if (pgs->program_infos[i].index == program_num)
		{
			return i;
		}
	}

	printf("invalid program num:'%d'", program_num);

	return -1;
}

static int prepareDemod(PGShandlerT *pgs)
{
    /* not do in PGS */
    return 0;
#if 0
    int ret;
    pgs->demod = DC_DemodCreate();
    if (!pgs->demod)
    {
        printf("error.\n");
        goto err_out;
    }

    ret = DC_DemodInit(pgs->demod);
    if (ret != 0)
    {
        printf("error.\n");
        goto err_out;
    }

    ret = DC_DemodSetFrequency(pgs->demod, pgs->freq);
    if (ret != 0)
    {
        printf("error.\n");
        goto err_out;
    }

    return 0;

err_out:

    return -1;
#endif
}

static int request_buffer(void * arg, void *cookie)
{
    awesome_filter_info *info = (awesome_filter_info*)cookie;
    md_buf_t *pbuffer = (md_buf_t*) arg;

	pbuffer->buf	 =   info->buffer;
	pbuffer->buf_size =  info->buf_size;
	return 0;
}

static int update_data(void * arg, void *cookie)
{
    unsigned int len = *((unsigned int *)arg);
    awesome_filter_info *info = (awesome_filter_info*)cookie;
    /* handle packets */
    if (len && info->buffer && info->pgs->pool)
    {
        TSBufferT *tsbuf = TSBufferCreate(info->pgs->pool, info->buffer, len);
#ifdef PGS_DEBUG
        DEBUG_DumpTS_Write(info->buffer, TS_PACKET_SIZE, len / TS_PACKET_SIZE);
#endif
        CdxQueuePush(info->pgs->buf_queue, tsbuf);
    }

    return 0;
}

static void initPGSParam(int type,void *cookie,demux_filter_param_t *param)
{
    param->cookie = cookie;
    param->stream_type = type;
    param->request_buffer_cb = request_buffer;
    param->update_data_cb = update_data;

#if defined(USE_AW_TSDEMUX)
    param->filter_type = DMX_FILTER_TYPE_TS;
#else
    param->filter_type = DMX_FILTER_TYPE_ALLTSPASS;
#endif
}

awesome_filter_info awesome_filter;
static int awesomeFilterInit(PGShandlerT *pgs)
{
    int ret = 0;
    if(pgs == NULL)
    {
        printf("PGS is NULL\n");
        return -1;
    }
    memset(&awesome_filter, 0, sizeof(awesome_filter_info));
    awesome_filter.buffer = (unsigned char *)malloc(TSC_LENGTH);
    if(awesome_filter.buffer == NULL)
    {
        printf("buffer malloc error\n\n");
        return -1;
    }

    awesome_filter.buf_size = TSC_LENGTH;
    awesome_filter.pgs = pgs;

#if defined(USE_AW_TSDEMUX)
    awesome_filter.tsc_hdr = TSDMX_SingletonGetInstance();
#else
    awesome_filter.tsc_hdr = TSDMX_vs_SingletonGetInstance();
    //set demux input source type
    demux_input_source_t input_source = DMX_INPUT_SOURCE_UNKNOW;
    switch(pgs->stdType)
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
    printf("pgs->stdType=%d\n", pgs->stdType);
    TSDMX_vs_setInputSource(input_source);
#endif

    if(awesome_filter.tsc_hdr == NULL)
    {
        printf("awesome filter TSDMX_SingletonGetInstance error\n\n");
        free(awesome_filter.buffer);
        awesome_filter.buffer = NULL;
        return -1;
    }

    printf("awsomeFilter Init Finish\n\n");
    return ret;
}

static void awsomeFilterDeinit(void)
{
    if(awesome_filter.buffer)
    {
        free(awesome_filter.buffer);
        awesome_filter.buffer = NULL;
    }
    awesome_filter.buf_size = 0;
    awesome_filter.pgs = NULL;
    if(awesome_filter.tsc_hdr)
    {
#if defined(USE_AW_TSDEMUX)
        TSDMX_Destroy(awesome_filter.tsc_hdr);
#else
        TSDMX_vs_Destroy(awesome_filter.tsc_hdr);
#endif
        awesome_filter.tsc_hdr = NULL;
    }
    printf("awsomeFilter Deinit Finish\n\n");
}

static int prepareTSC(PGShandlerT *pgs)
{
    int ret = 0;

#ifdef USE_TSC_DEV
    chan_register_t chan_register;

    pgs->tsc_hdr = tsc_dev_open();
    printf("prepareTSC tsc open !\n\n");
    chan_register.chan_type = CHAN_TYPE_LIVE;
    chan_register.pid = PID_ALL;
    chan_register.desc_id = 0;
    chan_register.callback = NULL;
    chan_register.callbackparam = NULL;
    chan_register.stream_type = TS_STREAM_UNKOWN;
    chan_register.ts_input_port = 0;
    chan_register.tsf_port = 0;
    chan_register.use_tsg = 0;

    pgs->chan = pgs->tsc_hdr->open_channel(pgs->tsc_hdr->cookie, &chan_register);
    if (pgs->chan < 0)
    {
        ret = -1;
    }
#else
    demux_filter_param_t PGSparam;
    ret = awesomeFilterInit(pgs);
    initPGSParam(DMX_STREAM_UNKOWN, &awesome_filter, &PGSparam);
    if(ret == 0)
    {
#if defined(USE_AW_TSDEMUX)
        ret = ts_demux_open_filter(awesome_filter.tsc_hdr, PID_ALL, &PGSparam);
#else
        ret = ts_demux_vs_open_filter(awesome_filter.tsc_hdr, &PGSparam);
#endif
        if(ret != 0)
        {
            printf("TSC open filter fail!\n\n\n");
#if defined(USE_AW_TSDEMUX)
            TSDMX_Destroy(awesome_filter.tsc_hdr);
#else
            TSDMX_vs_Destroy(awesome_filter.tsc_hdr);
#endif
        }
    }
#endif
    return ret;
}

static int doFinish(PGShandlerT *pgs)
{
    if (pgs->listener && pgs->listener->onFinish)
    {
        if (pgs->state == PGS_STATE_FINISHED)
        {
            pgs->listener->onFinish(pgs->cookie, PGS_ERR_OK, pgs->program_infos, pgs->success_program_cnt);
        }
        else if (pgs->state == PGS_STATE_TIMEOUT)
        {
            pgs->listener->onFinish(pgs->cookie, PGS_ERR_TIMEOUT, pgs->program_infos, pgs->success_program_cnt);
        }
        else if (pgs->state == PGS_STATE_FAULT)
        {
            pgs->listener->onFinish(pgs->cookie, PGS_ERR_FAULT, NULL, 0);
        }
        else if (pgs->state == PGS_STATE_STOP)
        {
            /* do nothing */
        }
        else
        {
            CDX_CHECK(0); /* state error, shoule not go here... */
        }
    }

#ifdef USE_TSC_DEV
    if(pgs->tsc_hdr)
    {
        pgs->tsc_hdr->close_channel(pgs->tsc_hdr->cookie, pgs->chan);
        printf("doFinish tsc close_channel finish !\n\n");

        tsc_dev_close(pgs->tsc_hdr);
        printf("doFinish tsc closed finish!\n\n");
        pgs->tsc_hdr = NULL;
    }
#else
    if(awesome_filter.tsc_hdr)
    {
#if defined(USE_AW_TSDEMUX)
        ts_demux_close_filter(awesome_filter.tsc_hdr, PID_ALL);
#else
        ts_demux_vs_close_filter(awesome_filter.tsc_hdr, DMX_FILTER_TYPE_ALLTSPASS);
#endif
        printf("doFinish tsc close filter finish !\n");
        awsomeFilterDeinit();
    }
#endif


    int i = 0;
    for (; i < 8192; i++)
    {
        if (pgs->tsp_pool->ts_arrs[i].data_came)
        {
            struct TsPackageS *tsp, *ntsp;
            CdxListForEachEntrySafe(tsp, ntsp, &pgs->tsp_pool->ts_arrs[i].list, node)
            {
                CdxListDel(&tsp->node);
                deleteTsPackage(pgs->pool, tsp);
            }
        }
    }
	//PGS_Destroy(pgs);
    return 0;
}

#ifdef PGS_DEBUG
static FILE *debug_dump_fd = NULL;

static void DEBUG_DumpTS_Write(void *ptr, size_t size, size_t nmemb)
{
    if (!debug_dump_fd)
    {
        debug_dump_fd = fopen("/mnt/pgs_debug_dump.ts", "w+\n");
    }
    CDX_CHECK(debug_dump_fd != NULL);

    size_t w_ret = 0, w_num = 0;

    do
    {
        w_ret = fwrite((char *)ptr + (size * w_num), size, (nmemb - w_num), debug_dump_fd);
        w_num += w_ret;

        if (w_num == nmemb)
        {
            break;
        }
        else
        {
            printf("write not finish, continue...\n");
        }

    } while (1);

    return;
}

static void DEBUG_DumpTS_Close()
{
    if (debug_dump_fd)
    {
        fclose(debug_dump_fd);
        debug_dump_fd = NULL;
    }
    return ;
}

#endif

#ifdef USE_TSC_DEV
static int fetchTSData(PGShandlerT *pgs)
{
    int ret = 0;

    tsf_data_t tsf_data;

    /* get data buffer pointer */
    ret = pgs->tsc_hdr->request_channel_data(pgs->tsc_hdr->cookie, pgs->chan, &tsf_data);
    if (ret != 0)
    {
        printf("something error ?\n");
        return -1;
    }

    /* handle packets */
    if (tsf_data.pkt_num)
    {
        TSBufferT *tsbuf = TSBufferCreate(pgs->pool, tsf_data.data, TS_PACKET_SIZE*tsf_data.pkt_num);
#ifdef PGS_DEBUG
        DEBUG_DumpTS_Write(tsf_data.data, TS_PACKET_SIZE, tsf_data.pkt_num);
#endif
        CdxQueuePush(pgs->buf_queue, tsbuf);
    }

    if (tsf_data.ring_pkt_num)
    {
        TSBufferT *tsbuf = TSBufferCreate(pgs->pool, tsf_data.ring_data, TS_PACKET_SIZE*tsf_data.ring_pkt_num);
#ifdef PGS_DEBUG
        DEBUG_DumpTS_Write(tsf_data.ring_data, TS_PACKET_SIZE, tsf_data.ring_pkt_num);
#endif
        CdxQueuePush(pgs->buf_queue, tsbuf);
    }

    pgs->tsc_hdr->flush_channel_data(pgs->tsc_hdr->cookie, pgs->chan, tsf_data.pkt_num + tsf_data.ring_pkt_num);

    return tsf_data.pkt_num + tsf_data.ring_pkt_num;
}
#endif

static int doDeliver(PGShandlerT *pgs)
{
    TSBufferT *buf = NULL;
    int buf_size;
    uint8_t *buf_ptr;
    int buf_cnt = 0;

    while ((buf = CdxQueuePop(pgs->buf_queue)) != NULL)
    {
        buf_cnt++;
        buf_size = TSBufferGetbufsize(buf);
        buf_ptr = TSBufferGetbufptr(buf);
        int offset = 0;
        uint16_t pid;
        for (offset = 0; offset < buf_size; offset += TS_PACKET_SIZE)
        {
            pid = TS_GetPID(buf_ptr + offset);

            if (pgs->tsp_pool->ts_arrs[pid].data_came == 0)
            {
                CdxListInit(&(pgs->tsp_pool->ts_arrs[pid].list));
            }
            if (pgs->tsp_pool->ts_arrs[pid].data_came < 5)
            {
                //printf("NTP come PID:'%d', count:'%d'", pid,
                //         pgs->tsp_pool->ts_arrs[pid].data_came);
                struct TsPackageS *tsp = newTsPackage(pgs->pool, buf, offset);
                CdxListAddTail(&tsp->node, &pgs->tsp_pool->ts_arrs[pid].list);
                pgs->tsp_pool->ts_arrs[pid].data_came++;
            }
            /* else drop */
        }

        TSBufferDecref(buf);
        /* parse PID */
    }

    return buf_cnt;
}

static int TS_ParseHeader(CdxBitReaderT *br, struct TS_HeaderS *ts_hdr)
{
    int ret = 0;

    ts_hdr->sync_byte = CdxBitReaderGetBits(br, 8);
    if (ts_hdr->sync_byte != 0x47)
    {
        printf("sync_byte err '%u', bad package!", ts_hdr->sync_byte);
        ret = -1;
        goto out;
    }

    ts_hdr->transport_error_indicator = CdxBitReaderGetBits(br, 1);
    if (ts_hdr->transport_error_indicator)
    {
        printf("err package, pls drop it.\n");
        ret = -1;
        goto out;
    }

    CdxBitReaderSkipBits(br, 2);
    /*
     * payload_unit_start_indicator:1
     * transport_priority:1
    */
    ts_hdr->PID = CdxBitReaderGetBits(br, 13);

    CdxBitReaderSkipBits(br, 2);
	/* transport_scrambling_control:2 */

    ts_hdr->adaption_field_control = CdxBitReaderGetBits(br, 2);

    ts_hdr->continuity_counter = CdxBitReaderGetBits(br, 4); /* continuity_counter:4 */
out:
    return ret;
}

static int TS_ParseAdaptfield(CdxBitReaderT *br, struct TS_AdaptionFieldS *adaption_field)
{
    int ret = 0;

    adaption_field->adaption_field_length = CdxBitReaderGetBits(br, 8);
    printf("adaption_field_length = %d", adaption_field->adaption_field_length);
    if (adaption_field->adaption_field_length > 183)
    {
        printf("Package error!\n");
        ret = -1;
        goto out;
    }

    // TODO: not support adaption field now.
    CdxBitReaderSkipBits(br, adaption_field->adaption_field_length*8);

out:
    return ret;
}

static int TS_ParsePAT(CdxBitReaderT *br, struct PAT_S *pat, AwPoolT *pool)
{
    int ret = 0;
    uint32_t section_off = 0;

    pat->table_id = CdxBitReaderGetBits(br, 8);
    if (pat->table_id != PAT_TID)
    {
        printf("Package error! table_ID: '%x'", pat->table_id);
        ret = -1;
        goto out;
    }

    CdxBitReaderSkipBits(br, 4);
    /*
     * uint32_t section_syntax_indicator:1
     * uint32_t zero:1
     * uint32_t reserved_1:2
     */

    pat->section_length = CdxBitReaderGetBits(br, 12);
    //printf("pat section length = %d", pat->section_length);
    if(PAT_SINGLE_SECTION_MAX < pat->section_length || 13 > pat->section_length)
    {
        printf("Package error! section error\n");
        ret = -1;
        goto out;
    }

    CdxBitReaderSkipBits(br, 40);
    /*
     *   uint32_t transport_stream_id:16;
     *   uint32_t reserved_2:2;
     *   uint32_t version_number:5;
     *   uint32_t current_next_indicator:1;
     *   uint32_t section_number:8;
     *   uint32_t last_section_number:8;
     */

    section_off = 5;

    pat->program_cnt = 0;
    CdxListInit(&pat->program_list);

    for (; section_off < (uint32_t)pat->section_length - CRC32_SIZE; section_off += 4)
    {
        struct PAT_ProgramInfoS *program_info = Palloc(pool, sizeof(*program_info));
        program_info->program_number = CdxBitReaderGetBits(br, 16);
        CdxBitReaderSkipBits(br, 3);  /* reserved:3 */
        program_info->program_map_PID = CdxBitReaderGetBits(br, 13);

        if (program_info->program_number != 0)
		{
	        pat->program_cnt++;
	        CdxListAddTail(&program_info->node, &pat->program_list);
        }
        else /* program_number = 0 */
        {
            Pfree(pool, program_info);
        }
    }

    pat->CRC_32 = CdxBitReaderGetBits(br, 32);
out:
    return ret;
}

static int TS_ParseDescriptor(CdxBitReaderT *br, uint32_t size, CdxListT *desc_list, AwPoolT *pool)
{
    uint32_t off = 0;
    int desc_cnt = 0;

    if(2 > size)
    {
        printf("descriptor length %d too short", size);
        return -1;
    }
    while (off < size - 2)
    {
        struct TS_DescriptorS *desc = Palloc(pool, sizeof(*desc));
        desc->descriptor_tag = CdxBitReaderGetBits(br, 8);
        desc->descriptor_length = CdxBitReaderGetBits(br, 8);
        if(off + desc->descriptor_length > size - 2)
        {
            printf("descriptor length error!\n");
            return -1;
        }
        memcpy(desc->data, CdxBitReaderData(br), desc->descriptor_length);
        CdxBitReaderSkipBits(br, desc->descriptor_length*8);

        desc_cnt++;
        CdxListAddTail(&desc->node, desc_list);

        //printf("off(%d), size(%d), descriptor_length(%d)", off, size, desc->descriptor_length);

        off += (desc->descriptor_length + 2);
    }

    return desc_cnt;
}

static int TS_ParsePMT(CdxBitReaderT *br, struct PMT_S *pmt, AwPoolT *pool)
{
    int ret = 0;
    uint32_t sect_off = 0;

    pmt->table_id = CdxBitReaderGetBits(br, 8);
    if (pmt->table_id != PMT_TID)
    {
        printf("Package error, table id:'%x'!", pmt->table_id);
        ret = -1;
        goto out;
    }

    CdxBitReaderSkipBits(br, 4);
    /*
     * uint32_t section_syntax_indicator:1
     * uint32_t zero:1
     * uint32_t reserved_1:2
     */

    pmt->section_length = CdxBitReaderGetBits(br, 12);
    //printf("pmt section length = %d", pmt->section_length);
    if(TS_PACKET_SIZE * PMT_SECTION_MAX_NUMBER < pmt->section_length || 18 > pmt->section_length)
    {
        printf("PMT Section Length error!\n");
        ret = -1;
        goto out;
    }

    pmt->program_number = CdxBitReaderGetBits(br, 16);

    CdxBitReaderSkipBits(br, 44);
    /*
      uint32_t reserved_2 :2;
      uint32_t version_number :5;
      uint32_t current_next_indicator :1;
      uint32_t section_number :8;
      uint32_t last_section_number :8;
      uint32_t reserved_3 :3;
      uint32_t PCR_PID :13;
      uint32_t reserved_4 :4;
      */

    pmt->program_info_length = CdxBitReaderGetBits(br, 12);
	if (pmt->program_info_length > 0)
	{
        if(pmt->program_info_length + 9 > pmt->section_length)
        {
            printf("PMT Program Info Length %d too long!", pmt->program_info_length);
            ret = -1;
            goto out;
        }
	    CdxListInit(&pmt->descriptor_list);
	    pmt->descriptor_cnt = TS_ParseDescriptor(br, pmt->program_info_length, &pmt->descriptor_list, pool);
        if(-1 == pmt->descriptor_cnt)
        {
            ret = -1;
            goto out;
        }
	}

    CdxListInit(&pmt->stream_list);
    sect_off = 9 + pmt->program_info_length;
    while (sect_off < (uint32_t)pmt->section_length - CRC32_SIZE)
    {
        struct PMT_StreamS *pmt_stream = Palloc(pool, sizeof(*pmt_stream));
        memset(pmt_stream, 0x00, sizeof(*pmt_stream));
        pmt_stream->stream_type = CdxBitReaderGetBits(br, 8);
        CdxBitReaderSkipBits(br, 3);
        pmt_stream->elementary_PID = CdxBitReaderGetBits(br, 13);
        CdxBitReaderSkipBits(br, 4);
        pmt_stream->ES_info_length = CdxBitReaderGetBits(br, 12);
        if(sect_off + 5 + pmt_stream->ES_info_length > (uint32_t)pmt->section_length - CRC32_SIZE)
        {
            printf("ES Info Length %d too long!", pmt_stream->ES_info_length);
            ret = -1;
            goto out;
        }

        pmt_stream->descriptor_cnt = 0;
        if (pmt_stream->ES_info_length > 0)
        {
            CdxListInit(&pmt_stream->descriptor_list);
            pmt_stream->descriptor_cnt = TS_ParseDescriptor(br, pmt_stream->ES_info_length, &pmt_stream->descriptor_list, pool);

			struct TS_DescriptorS *desc, *tempDesc;
			CdxListForEachEntrySafe(desc, tempDesc, &pmt_stream->descriptor_list, node)
			{
				CdxListDel(&desc->node);
				Pfree(pool, desc);
			}
		}
        CdxListAddTail(&pmt_stream->node, &pmt->stream_list);
        sect_off += (5 + pmt_stream->ES_info_length);
    }

    pmt->CRC_32 = CdxBitReaderGetBits(br, 32);

out:
    return ret;
}

static struct SDTSubpacketS SDTSubpacket;
static int TS_ParseSDT(CdxBitReaderT *br, struct SDT_S *sdt, AwPoolT *pool)
{
    int ret = 0;
    uint32_t sect_off = 0;

    sdt->table_id = CdxBitReaderGetBits(br, 8);
    if (sdt->table_id != SDT_TID)
    {
        printf("Package error!\n");
        ret = -1;
        goto out;
    }

    CdxBitReaderSkipBits(br, 4);
    /*
     * uint32_t section_syntax_indicator:1
     * uint32_t reserved_future_use_1:1
     * uint32_t reserved_1:2
     */

    sdt->section_length = CdxBitReaderGetBits(br, 12);

    printf("SDT, section_len:'%d'", sdt->section_length);
    if(12 > sdt->section_length)
    {
        printf("SDT Section Length too short!\n");
        ret = -1;
        goto out;
    }
    CdxBitReaderSkipBits(br, 64);
    /*
     uint32_t transport_stream_id :16;
     uint32_t reserved_2 :2;
     uint32_t version_number :5;
     uint32_t current_next_indicator :1;
     uint32_t section_number :8;
     uint32_t last_section_number :8;
     uint32_t original_network_id :16;
     uint32_t reserved_future_use_2 :8;
     */
    if(sdt->section_length > SDT_SINGLE_SECTION_MAX)//4:header; 1:pointer_field; 3:tableID,section_syntax_indicator,reserved_future_use_1,reserved_1
    {
        printf("SDT has subpacket!!!\n");
        SDTSubpacket.section_length = sdt->section_length - 8;
        SDTSubpacket.packet_sum = (sdt->section_length - SDT_SINGLE_DESCTIPTOR_TO_CRC_MAX) / TS_SUBPACKET_MAX;
        SDTSubpacket.packet_sum_left = (sdt->section_length - SDT_SINGLE_DESCTIPTOR_TO_CRC_MAX) % TS_SUBPACKET_MAX;
        uint8_t i;
        for(i = 0;i < SDT_SINGLE_DESCTIPTOR_TO_CRC_MAX;i++)
        {
            SDTSubpacket.descriptor_buf[i] = CdxBitReaderGetBits(br, 8);
        }
        SDTSubpacket.descriptor_buf_ptr = i;
        ret = 1;
    }
    else
    {
        SDTSubpacket.section_length = sdt->section_length - 8;
        SDTSubpacket.packet_sum = 0;
        SDTSubpacket.packet_sum_left = 0;
        uint8_t i;
        for(i = 0;i < (sdt->section_length - 8);i++)
        {
            SDTSubpacket.descriptor_buf[i] = CdxBitReaderGetBits(br, 8);
        }
        SDTSubpacket.descriptor_buf_ptr = i;
    }

#if 0
    sdt->service_cnt = 0;
    CdxListInit(&sdt->service_list);

    sect_off = 8;
    while (sect_off < sdt->section_length -4)
    {
        struct SDT_ServiceS *sdt_service = Palloc(pool, sizeof(*sdt_service));
        sdt_service->service_id = CdxBitReaderGetBits(br, 16);

        CdxBitReaderSkipBits(br, 12);
        /*
           uint32_t reserved_future_use :6;
           uint32_t EIT_schedule_flag :1;
           uint32_t EIT_present_following_flag :1;
           uint32_t running_status :3;
           uint32_t free_CA_mode :1;
           */
        sdt_service->descriptors_loop_length = CdxBitReaderGetBits(br, 12);
        printf("SDT, descriptors_loop_length(%d)", sdt_service->descriptors_loop_length);

        CdxListInit(&sdt_service->descriptor_list);
        sdt_service->descriptor_cnt = TS_ParseDescriptor(br,
                                            sdt_service->descriptors_loop_length,
                                            &sdt_service->descriptor_list,
                                            pool);

        sdt->service_cnt++;
        CdxListAddTail(&sdt_service->node, &sdt->service_list);

        sect_off += (5 + sdt_service->descriptors_loop_length);

        printf("SDT, sect_off:'%d', (%d)", sect_off, sdt_service->descriptors_loop_length);
    }

    sdt->CRC_32 = CdxBitReaderGetBits(br, 32);
#endif
out:
    return ret;
}

static int getServiceNameByDescList(CdxListT *desc_list, char *pName, int *pLen)
{
    struct TS_DescriptorS *desc;

    CdxListForEachEntry(desc, desc_list, node)
    {
        if (desc->descriptor_tag == TS_DESC_TAG_SERVICE)
        {
            printf("descriptor_length = %d", desc->descriptor_length);
            if(2 > desc->descriptor_length)
            {
                printf("descriptor_length too short\n");
                return -1;
            }
            CdxBitReaderT *br = CdxBitReaderCreate(desc->data, desc->descriptor_length);
            CdxBitReaderSkipBits(br, 8); /* service_type */
            uint32_t service_provider_name_length = CdxBitReaderGetBits(br, 8);
            printf("service_provider_name_length = %d", service_provider_name_length);
            uint32_t provider_at_least = 2 + service_provider_name_length + 1;//service_type:8,service_provider_name_length:8,service_name_length:8
            if(provider_at_least > desc->descriptor_length)
            {
                printf("service_provider_name_length too long\n");
                CdxBitReaderDestroy(br);
                return -1;
            }
            CdxBitReaderSkipBits(br, service_provider_name_length*8); /* service_provider_name */
            uint32_t service_name_length = CdxBitReaderGetBits(br, 8);
            printf("service_name_length = %d", service_name_length);
            if(provider_at_least + service_name_length> desc->descriptor_length)
            {
                printf("service_name_length too long\n");
                CdxBitReaderDestroy(br);
                return -1;
            }
            if (service_name_length > PROGRAM_NAME_LENGTH - 1)
            {
                service_name_length = PROGRAM_NAME_LENGTH - 1;
            }
            char name[PROGRAM_NAME_LENGTH] = {0};
            size_t outLen = PROGRAM_NAME_LENGTH;
            size_t NameLen = service_name_length;
            memcpy(name, CdxBitReaderData(br), service_name_length);
            gbk2utf8(name, &NameLen, pName, &outLen);
            //pName[service_name_length] = '\0';
            *pLen = service_name_length;

            CdxBitReaderDestroy(br);
            return 0;
        }
    }
    return -1;
}

static int getServiceMultiNameByDescList(CdxListT *desc_list, struct MultiNameLoopS *pName, int *pLen)
{
    struct TS_DescriptorS *desc;

    CdxListForEachEntry(desc, desc_list, node)
    {
        if (desc->descriptor_tag == TS_DESC_TAG_MULTI_NAME)
        {
            uint32_t loop_length = 0;
            uint8_t multi_name_count = 0;
            printf("descriptor_length = %d", desc->descriptor_length);
            if(4 > desc->descriptor_length)
            {
                printf("descriptor_length too short\n");
                return -1;
            }
            CdxBitReaderT *br = CdxBitReaderCreate(desc->data, desc->descriptor_length);
            do {
                memcpy(pName[multi_name_count].multi_language, CdxBitReaderData(br), PROGRAM_MULTI_LANGUAGE_LENTH);
                CdxBitReaderSkipBits(br, 24); /* ISO 639_language */
                loop_length += 24;
                uint32_t service_provider_multi_name_length = CdxBitReaderGetBits(br, 8);
                //printf("service_provider_multi_name_length = %d", service_provider_multi_name_length);
                uint32_t provider_at_least = 3 + 1 + service_provider_multi_name_length + 1;//ISO 639_language_code:24,service_provider_name_length:8,service_name_length:8
                if(provider_at_least > desc->descriptor_length)
                {
                    printf("service_provider_multi_name_length too long\n");
                    CdxBitReaderDestroy(br);
                    return -1;
                }
                loop_length += 8;
                CdxBitReaderSkipBits(br, service_provider_multi_name_length*8); /* service_provider_name */
                loop_length += service_provider_multi_name_length*8;
                uint32_t service_multi_name_length = CdxBitReaderGetBits(br, 8);
                //printf("service_multi_name_length = %d", service_multi_name_length);
                if(provider_at_least + service_multi_name_length > desc->descriptor_length)
                {
                    printf("service_provider_multi_name_length too long\n");
                    CdxBitReaderDestroy(br);
                    return -1;
                }
                loop_length += 8;
                if (service_multi_name_length > PROGRAM_NAME_LENGTH - 1)
                {
                    service_multi_name_length = PROGRAM_NAME_LENGTH - 1;
                }

                char multi_name[PROGRAM_NAME_LENGTH];
                memcpy(multi_name, CdxBitReaderData(br), service_multi_name_length);
                size_t multiNameLen = service_multi_name_length;
                size_t outLen = PROGRAM_NAME_LENGTH;
                //todo use verifytexttype
                if(multiNameLen % 2 == 1 && multi_name[0] == 0x11)
                {
                    multiNameLen--;
                    utf16utf8(&multi_name[1], &multiNameLen, pName[multi_name_count].multi_name, &outLen);
                }
                else
                {
                    char chs[] = "chs";
                    memcpy(pName[multi_name_count].multi_language, chs, PROGRAM_MULTI_LANGUAGE_LENTH);
                    gbk2utf8(multi_name, &multiNameLen, pName[multi_name_count].multi_name, &outLen);
                }
                loop_length += service_multi_name_length*8;
                //pName[multi_name_count].multi_name[multiNameLen] = '\0';
                multi_name_count++;
                *pLen = multi_name_count;
            }while(desc->descriptor_length > loop_length);
            CdxBitReaderDestroy(br);
            return 0;
        }
        else
        {
            //printf("MultiName Descriptor Tag error %x!", desc->descriptor_tag);
        }
    }
    return -1;
}


static TSStreamTypeT getStreamType(struct PMT_StreamS *pmt_stream)
{
    switch (pmt_stream->stream_type)
    {
    case PMT_STREAM_TYPE_MPEG1_P2_V:
    case PMT_STREAM_TYPE_MPEG2_P2_V:
    case PMT_STREAM_TYPE_MPEG4_P2_V:
    case PMT_STREAM_TYPE_MPEG_P10_V_AVC:
    case PMT_STREAM_TYPE_MPEG2_P2_V_DC:
    case PMT_STREAM_TYPE_V_VC1:
    case PMT_STREAM_TYPE_CAVS:
        return TS_STREAM_TYPE_VIDEO;

    case PMT_STREAM_TYPE_MPEG1_P3_A:
    case PMT_STREAM_TYPE_MPEG2_P3_A_MP3:
    case PMT_STREAM_TYPE_MPEG2_P7_A_AAC:
    case PMT_STREAM_TYPE_MPEG4_P3_A:
    case PMT_STREAM_TYPE_A_AC3:
    case PMT_STREAM_TYPE_A_EAC3:
    case PMT_STREAM_TYPE_MPEG2_P1_PRI:
        return TS_STREAM_TYPE_AUDIO;

    default:
        return TS_STREAM_TYPE_UNKNOW;

    }
    return TS_STREAM_TYPE_UNKNOW;
}

static int showProgramInfo(PGShandlerT *pgs)
{
    int i = 0;
    printf("Program num '%u'", pgs->program_cnt);
    for (; i < (int)pgs->program_cnt; i++)
    {
        printf("Program[%u]: '%s' '%u %u(1/%u)'", i, pgs->program_infos[i].name,
                pgs->program_infos[i].video_pid,
                pgs->program_infos[i].audio_pid[0], pgs->program_infos[i].audio_num);
    }
    return 0;
}

static int parseTSPackage_PAT(PGShandlerT *pgs, uint8_t *buf)
{
    int ret = 0;
    CdxBitReaderT *br = NULL;
    struct TS_HeaderS ts_hdr;
    struct TS_AdaptionFieldS adaption_field;
    struct PAT_S pat;

    printf("Parse PAT.\n");
    br = CdxBitReaderCreate(buf, TS_PACKET_SIZE);
    if (!br)
    {
        printf("err.\n");
        goto out;
    }

    ret = TS_ParseHeader(br, &ts_hdr);
    if (ret != 0)
    {
        printf("err.\n");
        goto out;
    }

    if (ts_hdr.adaption_field_control & 0x10)
    {
        ret = TS_ParseAdaptfield(br, &adaption_field);
        if (ret != 0)
        {
            printf("err.\n");
            goto out;
        }
    }

    CdxBitReaderSkipBits(br, 8); /* pointer_field:8 */

    ret = TS_ParsePAT(br, &pat, pgs->pool);
    if (ret != 0)
    {
        printf("err.\n");
        goto out;
    }

    pgs->program_cnt = pat.program_cnt;
    struct PAT_ProgramInfoS *program;
    uint32_t index = 0;
    printf("Program num:'%d'", pgs->program_cnt);

    CdxListForEachEntry(program, &pat.program_list, node)
    {
        pgs->program_infos[index].index = program->program_number;
        pgs->program_infos[index].pmt_pid = program->program_map_PID;

//        pgs->program_imap_tab[program->program_number] = index;

        printf("Program[%d]: num:'%d' PMT_PID'%d'",
                 index, program->program_number, program->program_map_PID);

        index++;
    }
	
	struct PAT_ProgramInfoS *program_info, *tmpProgram;
	CdxListForEachEntrySafe(program_info, tmpProgram, &pat.program_list, node)
	{
		CdxListDel(&program_info->node);
        Pfree(pgs->pool, program_info);
	}
	
    CDX_CHECK(index == pgs->program_cnt);

out:
    if (br)
    {
        CdxBitReaderDestroy(br);
    }
    return ret;
}

static int parseTSPackage_PMT(PGShandlerT *pgs, uint8_t *buf)
{
    int ret = 0;
    CdxBitReaderT *br = NULL;
    struct TS_HeaderS ts_hdr;
    struct TS_AdaptionFieldS adaption_field;
    struct PMT_S pmt;
	printf("Parse PMT.\n");

    memset(&pmt, 0x00, sizeof(pmt));

    br = CdxBitReaderCreate(buf, TS_PACKET_SIZE);
    if (!br)
    {
        printf("err.\n");
        goto out;
    }

    ret = TS_ParseHeader(br, &ts_hdr);
    if (ret != 0)
    {
        printf("err.\n");
        goto out;
    }

    printf("PMT, PID:'%d'.", ts_hdr.PID);
    if (ts_hdr.adaption_field_control & 0x2)
    {
        printf("PMT has adaption field\n");
        // TODO: not support adaption field now.
        //ret = TS_ParseAdaptfield(br, &adaption_field);
        //if (ret != 0)
        //{
        //    printf("err.\n");
        //    goto out;
        //}
    }
    CdxBitReaderSkipBits(br, 8); /* pointer_field:8 */

    ret = TS_ParsePMT(br, &pmt, pgs->pool);
    if (ret != 0)
    {
        printf("err.\n");
        goto out;
    }

    CDX_CHECK(pgs->program_cnt > 0);

//    int i = pgs->program_imap_tab[pmt.program_number];
	int i = programNum2Index(pgs, pmt.program_number);
    if(i < 0)
    {
        printf("not found index about program num %d !", (int)pmt.program_number);
        ret = -1;
        goto out;
    }
    struct PMT_StreamS *stream;
    CdxListForEachEntry(stream, &pmt.stream_list, node)
    {
        TSStreamTypeT stream_type = getStreamType(stream);
        if (stream_type == TS_STREAM_TYPE_VIDEO)
        {
            pgs->program_infos[i].video_pid = stream->elementary_PID;
            pgs->program_infos[i].video_codec_type = stream->stream_type;
        }
        else if (stream_type == TS_STREAM_TYPE_AUDIO)
        {
            int audio_stream_index = pgs->program_infos[i].audio_num;
            pgs->program_infos[i].audio_pid[audio_stream_index] = stream->elementary_PID;
            pgs->program_infos[i].audio_num++;
            pgs->program_infos[i].audio_codec_type = stream->stream_type;
        }
        else
        {
            printf("unknow support stream '%u'", stream->stream_type);
        }
    }

    if ((!pgs->program_infos[i].video_pid) && (pgs->program_infos[i].audio_num == 0))
    {
        printf("Parse PMT finish, but no stream found. \n");
        ret = -1;
    }

	struct PMT_StreamS *pmt_stream, *tmpPmt_stream;
	CdxListForEachEntrySafe(pmt_stream, tmpPmt_stream, &pmt.stream_list, node)
	{
		CdxListDel(&pmt_stream->node);
        Pfree(pgs->pool, pmt_stream);
	}

    /* parser PMT programInfo desctiptor loop*/
    if(pmt.program_info_length > 0)
    {
        struct TS_DescriptorS *pmt_desc;
        uint32_t loop_length = 0;
        CdxListForEachEntry(pmt_desc, &pmt.descriptor_list, node)
        {
            if (pmt_desc->descriptor_tag == TS_DESC_TAG_CA)
            {
                CdxBitReaderT *br = CdxBitReaderCreate(pmt_desc->data, pmt_desc->descriptor_length);
                do
                {
                    pgs->program_infos[i].scrambled = 1;
                    CdxBitReaderSkipBits(br, 16); /* CA_system_ID */
                    CdxBitReaderSkipBits(br, 3);  /* reserved */
                    CdxBitReaderSkipBits(br, 13); /* CA_PID */
                    loop_length += 4;
                }while(loop_length < pmt_desc->descriptor_length);

                CdxBitReaderDestroy(br);
                return 0;
            }
        }
        struct TS_DescriptorS *desc, *tempDesc;
        CdxListForEachEntrySafe(desc, tempDesc, &pmt.descriptor_list, node)
        {
            CdxListDel(&desc->node);
            Pfree(pgs->pool, desc);
        }
    }

out:
    if (br)
    {
        CdxBitReaderDestroy(br);
    }

    return ret;
}

static int parseTSPackage_SDT(PGShandlerT *pgs, uint8_t *buf)
{
    int ret = 0;
    CdxBitReaderT *br = NULL;
    struct TS_HeaderS ts_hdr;
    struct TS_AdaptionFieldS adaption_field;
    struct SDT_S sdt;
	struct SDT_ServiceS *sdt_service, *tempSdt_service;

    printf("Parse SDT.\n");
    if(pgs->program_cnt <= 0)
    {
        printf("We should parse PAT first!\n\n");
        goto out;
    }

    br = CdxBitReaderCreate(buf, TS_PACKET_SIZE);
    if (!br)
    {
        printf("error.\n");
        goto out;
    }

    ret = TS_ParseHeader(br, &ts_hdr);
    if (ret != 0)
    {
        printf("error.\n");
        goto out;
    }

    printf("Parse SDT, Header Finish.\n");

    // TODO: not support adaption field now.
#if 0
    if (ts_hdr.adaption_field_control & 0x10)
    {
		printf("Parse SDT, ParseAdaptfield begin.\n");
        ret = TS_ParseAdaptfield(br, &adaption_field);
        if (ret != 0)
        {
            printf("error.\n");
            goto out;
        }
		printf("Parse SDT, ParseAdaptfield Finish.\n");
    }
#endif

    printf("Parse SDT, TS_ParseSDT begin.\n");
    if(SDTSubpacket.packet_sum || SDTSubpacket.packet_sum_left)
    {
        if(((SDTSubpacket.current_subpacket_cnt + 1) & 0x0f) != ts_hdr.continuity_counter)
        {
            printf("error.Wrong SDT Subpacket!\n");
            ret = 1;
            goto out;
        }
        SDTSubpacket.current_subpacket_cnt = ts_hdr.continuity_counter;
        if(SDTSubpacket.packet_sum)
        {
            SDTSubpacket.packet_sum--;
            uint8_t i;
            for(i = 0;i < TS_SUBPACKET_MAX;i++)
            {
                SDTSubpacket.descriptor_buf[SDTSubpacket.descriptor_buf_ptr + i] = CdxBitReaderGetBits(br, 8);
            }
            SDTSubpacket.descriptor_buf_ptr += i;
            printf("Parse SDT, TS_ParseSDT [%d] Subpacket Handle.", SDTSubpacket.packet_sum);
            ret = 1;
        }
        else
        {
            uint8_t i;
            for(i = 0;i < SDTSubpacket.packet_sum_left;i++)
            {
                SDTSubpacket.descriptor_buf[SDTSubpacket.descriptor_buf_ptr + i] = CdxBitReaderGetBits(br, 8);
            }
            SDTSubpacket.descriptor_buf_ptr += SDTSubpacket.packet_sum_left;
            printf("Parse SDT, TS_ParseSDT Subpacket Finish.\n");
            ret = 0;
        }
    }
    else
    {
        CdxBitReaderSkipBits(br, 8); /* pointer_field:8 */
        ret = TS_ParseSDT(br, &sdt, pgs->pool);
        if (ret == -1)
        {
            printf("error.\n");
            goto out;
        }
        else if (ret == 1)
        {
            SDTSubpacket.current_subpacket_cnt = ts_hdr.continuity_counter;
            printf("Parse SDT, TS_ParseSDT First SubPacket Finish.\n");
            goto out;
        }
        else if (ret == 0)
        {
            printf("Parse SDT, TS_ParseSDT Finish.\n");
        }
    }

#if 0
    struct SDT_ServiceS *service;
    uint32_t success_name_cnt = 0;

    CdxListForEachEntry(service, &sdt.service_list, node)
    {
//        int i = pgs->program_imap_tab[service->service_id];
		int i = programNum2Index(pgs, service->service_id);
        int service_name_len;

        if (pgs->program_infos[i].name[0] != 0x0)
        {
            continue;
        }

        int ret1 = getServiceNameByDescList(&service->descriptor_list, pgs->program_infos[i].name, &service_name_len);
        if (ret1 == 0)
        {
            success_name_cnt++;
            printf("[%d]service name: '%s'", i, pgs->program_infos[i].name);

			struct TS_DescriptorS *desc, *tempDesc;
			CdxListForEachEntrySafe(desc, tempDesc, &service->descriptor_list, node)
			{
				CdxListDel(&desc->node);
				Pfree(pgs->pool, desc);
			}
        }
        else
        {
            printf("getServiceNameByDescList failure.\n");
        }
    }

    if (success_name_cnt != pgs->program_cnt)
    {
        uint32_t j = 0;
        for (; j < pgs->program_cnt; j++)
        {
            if (pgs->program_infos[j].name[0] == 0x0) /* check if any service not name. */
            {
                sprintf(pgs->program_infos[j].name, "unknow\n");
            }
        }
    }
#endif
out:
#if 0
	CdxListForEachEntrySafe(sdt_service, tempSdt_service, &sdt.service_list, node)
	{
		CdxListDel(&sdt_service->node);
        Pfree(pgs->pool, sdt_service);
	}
#endif
    if (br)
    {
        CdxBitReaderDestroy(br);
    }
    return ret;
}

static int parseSDTSubpacket(PGShandlerT *pgs)
{
    printf("parse SDT Descriptor Start\n");
    CdxBitReaderT *br = NULL;
    br = CdxBitReaderCreate(SDTSubpacket.descriptor_buf, SDTSubpacket.descriptor_buf_ptr);
    SDTSubpacket.service_cnt = 0;
    CdxListInit(&SDTSubpacket.service_list);
    uint32_t sect_off = 0;
    while (sect_off < SDTSubpacket.section_length - 4)
    {
        struct SDT_ServiceS *sdt_service = Palloc(pgs->pool, sizeof(*sdt_service));
        sdt_service->service_id = CdxBitReaderGetBits(br, 16);

        CdxBitReaderSkipBits(br, 6);
        /*
           uint32_t reserved_future_use :6;
        */
        sdt_service->EIT_schedule_flag = CdxBitReaderGetBits(br, 1);//EIT_schedule_flag :1
        sdt_service->EIT_present_following_flag = CdxBitReaderGetBits(br, 1);//EIT_present_following_flag :1
        sdt_service->running_status = CdxBitReaderGetBits(br, 3);//running_status :3
        sdt_service->free_CA_mode = CdxBitReaderGetBits(br, 1);//free_CA_mode :1

        sdt_service->descriptors_loop_length = CdxBitReaderGetBits(br, 12);
        printf("SDT, descriptors_loop_length(%d)", sdt_service->descriptors_loop_length);
        if(sect_off + 5 + sdt_service->descriptors_loop_length > SDTSubpacket.section_length - 4)
        {
            printf("SDT descriptor loop length %d too long", sdt_service->descriptors_loop_length);
            SDTSubpacket.service_cnt = -1;
            goto out;
        }
        CdxListInit(&sdt_service->descriptor_list);
        sdt_service->descriptor_cnt = TS_ParseDescriptor(br,
                                            sdt_service->descriptors_loop_length,
                                            &sdt_service->descriptor_list,
                                            pgs->pool);
        if(-1 == sdt_service->descriptor_cnt)
        {
            SDTSubpacket.service_cnt = -1;
            goto out;
        }
        SDTSubpacket.service_cnt++;
        CdxListAddTail(&sdt_service->node, &SDTSubpacket.service_list);

        sect_off += (5 + sdt_service->descriptors_loop_length);
        printf("SDT, sect_off:'%d', (%d)", sect_off, sdt_service->descriptors_loop_length);
    }

    SDTSubpacket.CRC_32 = CdxBitReaderGetBits(br, 32);
    printf("SDTSubpacket parser finish\n");
    struct SDT_ServiceS *service;
    uint32_t success_multi_name_cnt = 0;
    pgs->success_program_cnt = 0;
    CdxListForEachEntry(service, &SDTSubpacket.service_list, node)
    {
//        int i = pgs->program_imap_tab[service->service_id];
		int i = programNum2Index(pgs, service->service_id);
        pgs->program_infos[i].free_CA_mode = service->free_CA_mode;
        pgs->program_infos[i].running_status = service->running_status;
        pgs->program_infos[i].EIT_schedule_flag = service->EIT_schedule_flag;
        pgs->program_infos[i].EIT_present_following_flag = service->EIT_schedule_flag;
        int service_name_len;

        if (pgs->program_infos[i].name[0] != 0x0)
        {
            continue;
        }

        int ret2 = getServiceMultiNameByDescList(&service->descriptor_list, pgs->program_infos[i].multi_name_loop, &pgs->program_infos[i].multi_name_loop_count);
        if (ret2 == 0)
        {
            success_multi_name_cnt++;
            int j = 0;
            for(;j < pgs->program_infos[i].multi_name_loop_count;j++)
            {
                //printf("[%d]multi_name: '%s'", i, pgs->program_infos[i].multi_name_loop[j].multi_name + 1);
                for(int k = 0;k < 32;k++)
                {
                    //printf("[%d]multi_name: '%x'", i, pgs->program_infos[i].multi_name_loop[j].multi_name[k]);
                }
            }
            //printf("[%d]multi_name_loop_count: '%d'", i, pgs->program_infos[i].multi_name_loop_count);
        }
        else
        {
            printf("getServiceMultiNameByDescList failure.\n");
        }

        int ret1 = getServiceNameByDescList(&service->descriptor_list, pgs->program_infos[i].name, &service_name_len);
        if (ret1 == 0)
        {
            pgs->success_program_cnt++;
            //printf("[%d]service name: '%s'", i, pgs->program_infos[i].name);

			struct TS_DescriptorS *desc, *tempDesc;
			CdxListForEachEntrySafe(desc, tempDesc, &service->descriptor_list, node)
			{
				CdxListDel(&desc->node);
				Pfree(pgs->pool, desc);
			}
        }
        else
        {
            printf("getServiceNameByDescList failure.\n");
        }
    }

    if (pgs->success_program_cnt != pgs->program_cnt)
    {
        uint32_t j = 0;
        for (; j < pgs->program_cnt; j++)
        {
            if (pgs->program_infos[j].name[0] == 0x0) /* check if any service not name. */
            {
                sprintf(pgs->program_infos[j].name, "unknow\n");
            }
        }
    }

out:
    if (br)
    {
        CdxBitReaderDestroy(br);
    }
    struct SDT_ServiceS *sdt_service, *tempSdt_service;
    CdxListForEachEntrySafe(sdt_service, tempSdt_service, &SDTSubpacket.service_list, node)
	{
		CdxListDel(&sdt_service->node);
        Pfree(pgs->pool, sdt_service);
	}
    printf("service_cnt = %d", SDTSubpacket.service_cnt);
    printf("success_program_cnt = %d", pgs->success_program_cnt);
    return pgs->success_program_cnt;
}

static void *tsFetchComponent(void *args)
{
    int ret;
    PGShandlerT *pgs = (PGShandlerT *)args;
    int recv_pktnum = 0;

    printf("TS-Fetch-Comp Start...\n");

    ret = prepareDemod(pgs);
    if (ret != 0)
    {
        printf("error.\n");
        goto err_out;
    }

    ret = prepareTSC(pgs);
    if (ret != 0)
    {
        printf("error.\n");
        goto err_out;
    }

    while (PGS_STATE_RUNNING == pgs->state)
    {
        int64_t time_now = lGetNowUS();
        if (time_now - pgs->start_time > PGS_TIMEOUT_5S)
        {
            printf("PGS working TimeOUT. \n");
            pgs->state = PGS_STATE_TIMEOUT;
            break;
        }

#ifdef USE_TSC_DEV
        ret = fetchTSData(pgs);
        if (ret > 0)
        {
            recv_pktnum += ret;
            //printf("recv buf '%d'", recv_pktnum*188);
        }
        else
        {
            usleep(1000);/* no buffer, wait. */
        }
#endif
#ifdef PGS_DEBUG
        DEBUG_DumpTS_Close();
#endif

    }

    printf("TS-Fetch-Comp finish exit...\n");
    return NULL;

err_out:
    printf("TS-Fetch-Comp Error exit...\n");
    pgs->state = PGS_STATE_FAULT;
    return NULL;
}

static void *tsParseComponent(void *args)
{
    PGShandlerT *pgs = (PGShandlerT *)args;
    int ret;

    while (PGS_STATE_RUNNING == pgs->state)
    {
        ret = doDeliver(pgs);
        if (ret == 0)
        {
            usleep(70000); /* not get any buf, sleep wait */
            continue;
        }

        /* parse PAT */
        if (!(pgs->status_flags & PGS_STATUS_PAT_F))
        {
            if (pgs->tsp_pool->ts_arrs[PAT_PID].data_came)
            {
                struct TsPackageS *tsp, *tsp_next;
                CdxListForEachEntrySafe(tsp, tsp_next, &pgs->tsp_pool->ts_arrs[PAT_PID].list, node)
                {
                    ret = parseTSPackage_PAT(pgs, tsp->ptr);
                    if (ret == 0)
                    {
                        printf("Parse PAT finish...\n");
                        pgs->status_flags |= PGS_STATUS_PAT_F;
                        break;
                    }

					CdxListDel(&tsp->node);
					pgs->tsp_pool->ts_arrs[PAT_PID].data_came--;
					deleteTsPackage(pgs->pool, tsp);
                }
            }
        }

        /* parse PMT */
        if ((pgs->status_flags & PGS_STATUS_PAT_F)
            && (!(pgs->status_flags & PGS_STATUS_PMT_F)))
        {
            uint32_t i = 0;
            for (; i < pgs->program_cnt; i++)
            {
                uint32_t pid = pgs->program_infos[i].pmt_pid;

                if ((!pgs->program_infos[i].valid) && pgs->tsp_pool->ts_arrs[pid].data_came)
                {
                    struct TsPackageS *tsp, *tsp_next;
                    CdxListForEachEntrySafe(tsp, tsp_next, &pgs->tsp_pool->ts_arrs[pid].list, node)
                    {
                        ret = parseTSPackage_PMT(pgs, tsp->ptr);
                        if (ret == 0)
                        {
                            pgs->program_infos[i].valid = 1;
                            pgs->valid_program_cnt++;
                            break;
                        }

						CdxListDel(&tsp->node);
						pgs->tsp_pool->ts_arrs[pid].data_came--;
						deleteTsPackage(pgs->pool, tsp);
                    }
                }

            }

            if (pgs->valid_program_cnt == pgs->program_cnt) /* ALL PMT parse finish */
            {
                printf("Parse PMT finish...\n");
                pgs->status_flags |= PGS_STATUS_PMT_F;
                SDTSubpacket.CRC_32 = 0;
                memset(SDTSubpacket.descriptor_buf, 0, SDT_DESCRIPTOR_MAX);
                SDTSubpacket.descriptor_buf_ptr = 0;
                SDTSubpacket.packet_sum = 0;
                SDTSubpacket.packet_sum_left = 0;
                SDTSubpacket.section_length = 0;
                SDTSubpacket.current_subpacket_cnt = 0;
            }
        }

        /* parse SDT */
        if ((pgs->status_flags & PGS_STATUS_PAT_F)
            && (pgs->status_flags & PGS_STATUS_PMT_F)
            && (!(pgs->status_flags & PGS_STATUS_SDT_F)))
        {
            if (pgs->tsp_pool->ts_arrs[SDT_PID].data_came)
            {
				struct TsPackageS *tsp, *tsp_next;
                CdxListForEachEntrySafe(tsp,tsp_next, &pgs->tsp_pool->ts_arrs[SDT_PID].list, node)
                {
                    ret = parseTSPackage_SDT(pgs, tsp->ptr);
                    if (0 == ret)
                    {
                        ret = parseSDTSubpacket(pgs);
                        if(pgs->program_cnt == ret)
                        {
                            printf("Parse SDT finish...\n");
                            pgs->status_flags |= PGS_STATUS_SDT_F;
                            break;
                        }
                    }
					CdxListDel(&tsp->node);
					pgs->tsp_pool->ts_arrs[SDT_PID].data_came--;
					deleteTsPackage(pgs->pool, tsp);
                }
            }
        }

        if (pgs->status_flags == PGS_STATUS_ALLDONE)
        {
            printf("Awesome Program search do finish, cost(%.2f)...",
                    (lGetNowUS()-pgs->start_time)/1000000.0);
            showProgramInfo(pgs);
            pgs->state = PGS_STATE_FINISHED;
            break;
        }
    }

    doFinish(pgs);
    return NULL;
}

/* disposable OBJ, should only USE one time. */
int PGS_Start(PGShandlerT *pgs)
{
    int ret = 0;

    if(pgs->state == PGS_STATE_RUNNING)
    {
        printf("PGS already in runnig state ,return !\n");
        return 0;
    }

    pgs->state = PGS_STATE_RUNNING;
    pgs->start_time = lGetNowUS();

    printf("PGS_Start\n\n");

    ret = pthread_create(&pgs->fetch_thread, NULL, tsFetchComponent, pgs);
    if (ret != 0)
    {
        printf("error.\n");
        pgs->state = PGS_STATE_FAULT;
        return -1;
    }

    ret = pthread_create(&pgs->parse_thread, NULL, tsParseComponent, pgs);
    if (ret != 0)
    {
        printf("error.\n");
        pgs->state = PGS_STATE_FAULT;
        return -1;
    }

    return 0;
}

int PGS_Stop(PGShandlerT *pgs)
{
    if(pgs->state == PGS_STATE_STOP)
    {
        printf("PGS already in stop state ,return !\n");
        return 0;
    }

    pgs->state = PGS_STATE_STOP;

    /* wait this 2 thread exit */
    if(pgs->fetch_thread)
    {
        pthread_join(pgs->fetch_thread, NULL);
    }

    if(pgs->parse_thread)
    {
        pthread_join(pgs->parse_thread, NULL);
    }

    printf("PGS_Stop finish !\n");
    return 0;
}

int PGS_Destroy(PGShandlerT *pgs)
{
#if 1
    if(pgs->tsp_pool)
    {
        Pfree(pgs->pool,pgs->tsp_pool);
        pgs->tsp_pool = NULL;
    }

    if(pgs->buf_queue)
    {
    	TSBufferT *buf = NULL;
		while ((buf = CdxQueuePop(pgs->buf_queue)) != NULL)
		{
			TSBufferDecref(buf);
		}
        CdxQueueDestroy(pgs->buf_queue);
        pgs->buf_queue = NULL;
    }

    if(pgs->pool)
    {
        AwPoolDestroy(pgs->pool);
        pgs->pool = NULL;
    }
#endif

    if (pgs)
    {
        free(pgs);
        pgs = NULL;
    }

    printf("PGS_Destroyfinish !\n");
    return 0;
}

PGShandlerT *PGS_Instance(struct PGS_ListenerS *listener, void *cookie, uint32_t freq, DvbStandardType type)
{
    struct PGSctxS *pgs = malloc(sizeof(*pgs));

    memset(pgs, 0x00, sizeof(*pgs));

	//log_set_level(LOG_LEVEL_DEBUG);

    printf("PGS_Instance, type=%d\n", type);

    pgs->pool = AwPoolCreate(NULL);
    pgs->buf_queue = CdxQueueCreate(pgs->pool);

    pgs->tsp_pool = Palloc(pgs->pool, sizeof(struct TsPackagePoolS));
    memset(pgs->tsp_pool, 0x00, sizeof(struct TsPackagePoolS));

    pgs->freq = freq;
    pgs->listener = listener;
    pgs->cookie = cookie;
    pgs->stdType = type;

    return pgs;
}

