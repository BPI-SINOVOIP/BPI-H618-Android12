#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "tsdemux_vs.h"
#include "tsdemux_i_vs.h"
#include "trid_util.h"
#include "thal_cpucomm_api.h"
#include "trid_datatype.h"
#include "thal_demux_api.h"
#include "dtmbip_sunxi.h"
#include "dtv_ion_util.h"

#define CIP_BUFFERSIZE  (4 * 1024 * 1024)
#define TSPACKET_SIZE   (188)
#define AV_RB16(p)      ((*(p)) << 8 | (*((p) + 1)))

#if SAVE_DEMUX_STREAM
FILE *demux_record_file = NULL;
FILE *demux_cip_file = NULL;
#endif

struct dmx_device_manager
{
    void* handle;
    int ref;
    int path_ref;
    demux_input_source_t input_source;
    tridDataPathHandle_t path_handle;
    pthread_mutex_t mutex;
};

/* save global demux device handle */
static struct dmx_device_manager dmx_mgr =
{
    .handle = NULL,
    .ref = 0,
    .input_source = DMX_INPUT_SOURCE_UNKNOW,
    .path_ref = 0,
    .path_handle = DMX_INVALID_HANDLE,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};


#if ENABLE_FILTER_MULTIPLE
allts_filter_t allts_filter_ext;
pthread_mutex_t allts_mutex_ext;

static void reset_ext_filter(allts_filter_t *filter)
{
    if(filter)
    {
        pthread_mutex_lock(&allts_mutex_ext);
        filter->cookie = NULL;
        filter->requestbufcb = NULL;
        filter->updatedatacb = NULL;
        filter->push_allts_data = NULL;
        filter->data_offset = 0;
        filter->frame_complete = TRUE;
        pthread_mutex_unlock(&allts_mutex_ext);
    }
    printf("reset multiple filter finish\n");
}
#endif

static int logPID(uint8_t *buf)
{
    return ((buf[1]&0x1f) << 8) | buf[2];
}

static void *dtv_ion_Phy2VirAddr(unsigned long PhyStart, unsigned long PhyTarget, void *VirStart)
{
    unsigned long tmp;
    void *VirTarget;

    tmp = PhyTarget - PhyStart;
    VirTarget = VirStart + tmp;

    return VirTarget;
}

static int32_t get_input_source_type(filter_t *filter)
{
    int32_t tssource = -1;
    demux_input_source_t input_source = dmx_mgr.input_source;
    switch(input_source)
    {
        case DMX_INPUT_SOURCE_ATSC_DVB:
             if(filter->type == DMX_FILTER_TYPE_ALLTSPASS)
                tssource = CIP_INPUT_SOURCE_PARALLEL_ATSC_DVB;
             else
                tssource = INPUT_SOURCE_PARALLEL_ATSC_DVB;

              break;

        case DMX_INPUT_SOURCE_DTMB:
        default:
             if(filter->type == DMX_FILTER_TYPE_ALLTSPASS)
                tssource = CIP_INPUT_SOURCE_PARALLEL_DTMB;
             else
                tssource = INPUT_SOURCE_PARALLEL_DTMB;

             break;
    }

    printf("get input source type %d for filter %d !\n", tssource, filter->type);
    return tssource;
}

static int32_t push_allts_data(uint8_t *data, uint32_t len, uint32_t new_frm, void *param)
{
    (void)new_frm;
    uint8_t *dataptr = data;
    allts_filter_t *allts = (allts_filter_t *)param;
    uint32_t total_framelength = 0;

    if (len % TSPACKET_SIZE != 0)
    {
        allts->frame_complete = FALSE;
        if ((allts->data_offset + len) % TSPACKET_SIZE == 0)
        {
            allts->frame_complete = TRUE;
            //printf("frame_complete end soon! allts->data_offset=,%d,len=,%d\n", allts->data_offset, len);
        }
    }

    if (allts->frame_complete == FALSE)
    {
        /* The case: 1st frame is not align to 188 but 2rd is align
         * This case is abnormally and the data should be discard
         */
        if (len % TSPACKET_SIZE == 0)
        {
            //printf("frame_complete abnormally! allts->data_offset=,%d,len=,%d\n", allts->data_offset, len);
            allts->frame_complete = TRUE;
            allts->data_offset = 0;
            allts->cur_ptr = NULL;
            allts->free_size = 0;
            return 0;
        }

        //if (allts->frame_complete == FALSE)
            //printf("frame_complete start/continue! allts->data_offset=,%d,len=,%d\n", allts->data_offset, len);
    }

    while (len > 0)
    {
        if (!allts->data_offset)
        {
            allts->requestbufcb(&allts->md_buf, allts->cookie);
            if (allts->md_buf.buf == NULL)
            {
                printf("allts->requestbufcb is NULL fail!\n");
                return -1;
            }

            allts->cur_ptr = allts->md_buf.buf;
            allts->free_size = allts->md_buf.buf_size;
            //printf("ts->free_size: %d\n", allts->free_size);
        }

        if (allts->free_size < len)
        {
            memcpy(allts->cur_ptr, dataptr, allts->free_size);
            allts->updatedatacb(&allts->free_size, allts->cookie);
            len -= allts->free_size;
        }
        else
        {
            memcpy(allts->cur_ptr + allts->data_offset, dataptr, len);
            if (!allts->frame_complete)
            {
                allts->data_offset += len;
                allts->free_size -= len;
            }
            else
            {
                total_framelength = allts->data_offset + len;
                allts->updatedatacb(&total_framelength, allts->cookie);
                allts->data_offset = 0;
            }

            len = 0;
        }
        if (!allts->data_offset)
        {
            allts->cur_ptr = NULL;
            allts->free_size = 0;
        }
    }

    return 0;
}

static int32_t push_ts_data(uint8_t *data, uint32_t len, uint32_t new_frm, void *param)
{
    (void)new_frm;
    uint8_t *dataptr = data;
    ts_filter_t *ts = (ts_filter_t *)param;

    while (len > 0) {
        //request buffer
        ts->requestbufcb(&ts->md_buf, ts->cookie);
        if (ts->md_buf.buf == NULL) {
            return -1;
        }

        ts->cur_ptr = ts->md_buf.buf;
        ts->free_size = ts->md_buf.buf_size;
        //printf("ts->free_size: %d\n", ts->free_size);
        if (ts->free_size < len) {
            memcpy(ts->cur_ptr, dataptr, ts->free_size);
            ts->updatedatacb(&ts->free_size, ts->cookie);
            len -= ts->free_size;
        } else {
            memcpy(ts->cur_ptr, dataptr, len);
            ts->updatedatacb(&len, ts->cookie);
            len = 0;
        }
        ts->free_size = 0;
        ts->cur_ptr = NULL;
    }

    return 0;
}

static int32_t push_section_data(uint8_t *data, uint32_t len, uint32_t new_section, void *param)
{
    uint8_t *dataptr = data;
    section_filter_t *tss = (section_filter_t*) param;

    if (new_section) {
        tss->free_size = 0;
        tss->cur_ptr = NULL;
        if (tss->section_h_size > 0) {
            if (tss->end_of_section_reached != 0) {
                //find next section,
                //but the last section has not closed, discard the parsed datas
                tss->updatedatacb(&tss->section_h_size, tss->cookie);
                tss->free_size = 0;
            } else {
                tss->cur_ptr = tss->md_buf.buf;
                tss->free_size = tss->md_buf.buf_size;
            }
        }

        tss->end_of_section_reached = 0;
        tss->section_index = 0;
        //section_h_size - PMT/PAT... total length(tableid to CRX32) exclude PSI_pointer_field and Packet heard
        tss->section_h_size = (AV_RB16(dataptr + 1) & 0xfff) + 3;
        if (tss->section_h_size > MAX_SECTION_SIZE) {
            tss->section_h_size = MAX_SECTION_SIZE;
        }
    }

    while (len > 0) {
        if (tss->free_size == 0) {
            //request buffer
            tss->requestbufcb(&tss->md_buf, tss->cookie);
            if (tss->md_buf.buf == NULL) {
                return -1;
            }

            tss->cur_ptr = tss->md_buf.buf;
            tss->free_size = tss->md_buf.buf_size;
        }

        if (tss->free_size < len) {
            memcpy(tss->cur_ptr, dataptr, tss->free_size);
            tss->cur_ptr += tss->free_size;
            tss->section_index += tss->free_size;
            len = 0;

            tss->free_size = 0;
            tss->end_of_section_reached = 1;
            tss->updatedatacb(&tss->section_h_size, tss->cookie);
            tss->section_h_size = 0;
        } else {
            memcpy(tss->cur_ptr, dataptr, len);
            tss->free_size -= len;
            tss->cur_ptr += len;
            tss->section_index += len;
            len = 0;
        }

        if ((tss->section_h_size != 0)
                && (tss->section_index >= tss->section_h_size)) {
            tss->end_of_section_reached = 1;
            tss->updatedatacb(&tss->section_h_size, tss->cookie);
            tss->free_size = 0;
            tss->section_h_size = 0;
        }
    }

    return 0;
}

#if SAVE_DEMUX_STREAM
int saveTsDataProcess(FILE *fd, trid_uint8* buffer, trid_uint32 size) {
    int ret = -1;

    if (!buffer || !fd) {
        printf("RecordTsDataProcess get NULL buffer or NULL fd to write\n\n");
        return -1;
    }

    ret = fwrite(buffer, 1, size, fd);
    if (ret > 0) {
        printf("write data ,pid %d, buffer %x %x %x %x, size %d !\n", \
               logPID(buffer), buffer[0],buffer[1],buffer[2],buffer[3],size);
    } else {
        printf("UserSpace: TsDataProcess Error\n\n");
    }

    return 0;
}
#endif

static void *recorder_data_thread(void* arg) {
    unsigned int uBytesCount1 = 0, uBytesCount2 = 0, uBlockSize = TSPACKET_SIZE * 10;
    tridDmxCbufPointers_t bufferPointer;
    filter_t *filter = (filter_t *)arg;
    tridDataPathHandle_t datapathhandle = filter->vs_demux_handle;
    ts_filter_t *ts_filter = &(filter->u.ts_filter);
    uint8_t *pAckPtr = NULL;

    while (filter->status != DEMUX_STATUS_STOPPED) {
        uBytesCount1 = 0;
        uBytesCount2 = 0;

        if (SYS_NOERROR != Thal_Demux_GetRecordCbufPointers(datapathhandle, TRID_PVR_RD_RECORD, &bufferPointer)) {
            //printf(TL_INFO,"UserSpace: Get CBUF pointers failed,try again next loop\n\n");
            continue;
        }

        if (pAckPtr == NULL)
            pAckPtr = (uint8_t *)bufferPointer.readaddr;

        if ((unsigned int)pAckPtr <= bufferPointer.writeaddr) {
            uBytesCount1 = bufferPointer.writeaddr - (unsigned int)pAckPtr;
        } else {
            uBytesCount1 = bufferPointer.endaddr - (unsigned int)pAckPtr;
            uBytesCount2 = bufferPointer.writeaddr - bufferPointer.startaddr;
        }

        if (uBytesCount2 > 0)
            uBytesCount2 = uBytesCount2 / uBlockSize * uBlockSize;
        else
            uBytesCount1 = uBytesCount1 / uBlockSize * uBlockSize;

        if ((uBytesCount1 > 0) || (uBytesCount2 > 0)) {
            ts_filter->push_ts_data(pAckPtr, uBytesCount1, 0, (void *)ts_filter);
            //printf("########record data pid %d !\n" ,logPID(pAckPtr));
#if SAVE_DEMUX_STREAM
            saveTsDataProcess(demux_record_file, pAckPtr, uBytesCount1);
#endif
            if (uBytesCount2 > 0) {
                ts_filter->push_ts_data((uint8_t *)bufferPointer.startaddr, uBytesCount2, 0, (void *)ts_filter);
#if SAVE_DEMUX_STREAM
                saveTsDataProcess(demux_record_file, (trid_uint8*)bufferPointer.startaddr, uBytesCount2);
#endif
            }

            pAckPtr += (uBytesCount1 + uBytesCount2);
            if ((unsigned int)pAckPtr >= bufferPointer.endaddr)
                pAckPtr = (uint8_t *)((unsigned int)pAckPtr - bufferPointer.endaddr + bufferPointer.startaddr);

            printf("UserSpace: Update read pointer 0x%08x\n", (int)pAckPtr);
            if (SYS_NOERROR != Thal_Demux_UpdateRecordReadPointer(datapathhandle, (int)pAckPtr, TRID_PVR_RD_RECORD))
                printf("UserSpace: Update reader pointer failed,try again next loop\n\n");
        }

        /* sleep 5ms */
        usleep(5 * 1000);
    }

    printf("exit recorder_data_thread\n\n");
    pthread_exit(NULL);
    return NULL;
}

static trid_sint32 SectionProcess(tridDataPathHandle_t datapathhandle, tridFilterHandle_t filterhandle, trid_uint32 numberOfSections,
                                  tridDemuxData_t pSections[])
{
    trid_uint32 uI;
    void* buffer[MAX_FILTER_LIST_LENGTH];
    char* psectiondata = NULL;
    section_filter_t *section = NULL;
    mpegts_context_t *mp;

    if (dmx_mgr.handle != NULL) {
        mp = (mpegts_context_t *)dmx_mgr.handle;
        section = &(mp->filter[DMX_FILTER_TYPE_SECTION]->u.section_filter);
    } else {
        printf("%s can't get valid dmx_mgr.handle or section_filter handle\n", __func__);
        return -1;
    }

    for (uI = 0; uI < numberOfSections; uI++, pSections++) {
        buffer[uI] = pSections[uI].pbuffer;
        psectiondata = (char*)(pSections[uI].pbuffer);
#if 0
        printf("%s : numberOfSections %d,uI %d, crcstatus= (%d) length = (%d) data:%d : %d :%d :%d :%d \n", __FUNCTION__, (int)numberOfSections, (int)uI, (int)pSections->crcstatus,
               (int)pSections->length, (int)psectiondata[0], (int)psectiondata[1], (int)psectiondata[2], (int)psectiondata[3],
               (int)psectiondata[4]);
#endif
        section->push_section_data((void*)buffer[uI], pSections->length, 1, section);
    }

    Thal_Demux_FreeSecData(datapathhandle, filterhandle, numberOfSections, buffer);

    return (SYS_NOERROR);
}

static void *AllTsProcess(void* arg)
{
    tridDmxCbufPointers_t stCbufPointers;
    allts_filter_t *allts_filter = NULL;
    void* pdumpbuffer = NULL;
    filter_t *filter = (filter_t *)arg;
    unsigned int ilength = 0;
    unsigned int uwriteaddr, ureadaddr, ubeginaddr, uendaddr;

    allts_filter = &(filter->u.allts_filter);
    while (filter->status != DEMUX_STATUS_STOPPED)
    {
        Thal_CIP_GetCapBufPointers(filter->vs_demux_handle, &stCbufPointers);
        ubeginaddr = stCbufPointers.startaddr;
        uendaddr = stCbufPointers.endaddr;
        ureadaddr = stCbufPointers.readaddr;
        uwriteaddr = stCbufPointers.writeaddr;
        if(ureadaddr < ubeginaddr || ureadaddr > uendaddr || uwriteaddr > uendaddr)
        {
            printf("08-18,AllTsProcess, data addr error ,[ubeginaddr %x, uendaddr %x, ureadaddr %x, uwriteaddr %x] !\n", \
                  ubeginaddr, uendaddr, ureadaddr, uwriteaddr);
            usleep(2 * 1000);
            Thal_CIP_UpdateCapBufReadPointer(filter->vs_demux_handle, uwriteaddr);
            continue;
        }

        if (ureadaddr > uwriteaddr) {
            /* push new TS data with callback */
            ilength = uendaddr - ureadaddr;
#if TVFE_IOMMU_ENABLE
            pdumpbuffer = dtv_ion_Phy2VirAddr(ubeginaddr, ureadaddr, allts_filter->pbuffer_vir);
            dtv_ion_flush_cache(pdumpbuffer, ilength);
#else
            pdumpbuffer = Trid_Util_CPUComm_Convert2VirtualAddr((void *)ureadaddr);
#endif
            allts_filter->push_allts_data(pdumpbuffer, ilength, 0, (void *)allts_filter);

#if SAVE_DEMUX_STREAM
            saveTsDataProcess(demux_cip_file, pdumpbuffer, ilength);
#endif

#if ENABLE_FILTER_MULTIPLE
            pthread_mutex_lock(&allts_mutex_ext);
            if(allts_filter_ext.push_allts_data)
                allts_filter_ext.push_allts_data(pdumpbuffer, ilength, 0, (void *)&allts_filter_ext);
            pthread_mutex_unlock(&allts_mutex_ext);
#endif
            ilength = uwriteaddr - ubeginaddr;
#if TVFE_IOMMU_ENABLE
            pdumpbuffer = dtv_ion_Phy2VirAddr(ubeginaddr, ubeginaddr, allts_filter->pbuffer_vir);
            dtv_ion_flush_cache(pdumpbuffer, ilength);
#else
            pdumpbuffer = Trid_Util_CPUComm_Convert2VirtualAddr((void *)ubeginaddr);
#endif
            allts_filter->push_allts_data(pdumpbuffer, ilength, 0, (void *)allts_filter);

#if SAVE_DEMUX_STREAM
            saveTsDataProcess(demux_cip_file, pdumpbuffer, ilength);
#endif

#if ENABLE_FILTER_MULTIPLE
            pthread_mutex_lock(&allts_mutex_ext);
            if(allts_filter_ext.push_allts_data)
                allts_filter_ext.push_allts_data(pdumpbuffer, ilength, 0, (void *)&allts_filter_ext);
            pthread_mutex_unlock(&allts_mutex_ext);
#endif
            /* Update CIP module Read Pointer */
            Thal_CIP_UpdateCapBufReadPointer(filter->vs_demux_handle, uwriteaddr);
        } else if (ureadaddr < uwriteaddr) {
            ilength = uwriteaddr - ureadaddr;
#if TVFE_IOMMU_ENABLE
            pdumpbuffer = dtv_ion_Phy2VirAddr(ubeginaddr, ureadaddr, allts_filter->pbuffer_vir);
            dtv_ion_flush_cache(pdumpbuffer, ilength);
#else
            pdumpbuffer = Trid_Util_CPUComm_Convert2VirtualAddr((void *)ureadaddr);
#endif
            allts_filter->push_allts_data(pdumpbuffer, ilength, 0, (void *)allts_filter);

#if SAVE_DEMUX_STREAM
            saveTsDataProcess(demux_cip_file, pdumpbuffer, ilength);
#endif

#if ENABLE_FILTER_MULTIPLE
            pthread_mutex_lock(&allts_mutex_ext);
            if(allts_filter_ext.push_allts_data)
                allts_filter_ext.push_allts_data(pdumpbuffer, ilength, 0, (void *)&allts_filter_ext);
            pthread_mutex_unlock(&allts_mutex_ext);
#endif

            Thal_CIP_UpdateCapBufReadPointer(filter->vs_demux_handle, uwriteaddr);
        }
        else {
            usleep(2 * 1000);
        }
    }

    pthread_exit(NULL);

    return NULL;
}

static int32_t demux_build_section_filter(tridFilterHandle_t *pfilterhandle, tridDataPathHandle_t vs_demux_handle,
                                            int32_t upid, uint8_t uPattern)
{
    tridFilterHandle_t filterhandle = DMX_INVALID_HANDLE;
    trid_uint32 cookie = 0x123456;
    tridDmxSecFilterCfg_t stsecfiltercfg;

    Thal_Demux_ReqSecFilter(vs_demux_handle, &filterhandle, cookie);

    stsecfiltercfg.pid = upid;
    stsecfiltercfg.crcmode = CRCMODE_ACCEPTALLSECTIONS;
    stsecfiltercfg.bonetimeonly = trid_false;
    stsecfiltercfg.listlength = 1;
    memset(&stsecfiltercfg.ptn, 0, sizeof(stsecfiltercfg.ptn));

    stsecfiltercfg.ptn.ptnlen = 1;
    stsecfiltercfg.ptn.pattern[0] = uPattern;
    stsecfiltercfg.ptn.equalmask[0] = 0xff;
    stsecfiltercfg.ptn.notequalmask[0] = 0x00;
    printf("demux_start_section, pid 0x%x ,table_id 0x%x !\n",upid, uPattern);

    Thal_Demux_SetSecFilterCfg(vs_demux_handle, filterhandle, &stsecfiltercfg);
    Thal_Demux_InstallSecDataCB(vs_demux_handle, filterhandle, SectionProcess);
    Thal_Demux_EnableSecFilter(vs_demux_handle, filterhandle, trid_true);

    *pfilterhandle = filterhandle;

    return 0;
}

static int32_t demux_start_section(section_filter_t *section_filter, tridDataPathHandle_t vs_demux_handle, demux_filter_param_t* filter_param)
{
    int32_t i;
    int32_t *pids;
    uint8_t *tableids;
    uint8_t pidnum, tabldidnum;

    pids     = filter_param->pids;
    pidnum   = filter_param->pid_num;
    tableids = filter_param->tableids;
    tabldidnum = (filter_param->tableid_num > DMX_FILTER_SIZE) ? DMX_FILTER_SIZE : filter_param->tableid_num;

    memset(section_filter->tableids, 0, sizeof(section_filter->tableids));
    memset(section_filter->filterhandle, 0 , sizeof(section_filter->filterhandle));

    for(i = 0; i < tabldidnum; i++)
    {
        demux_build_section_filter(&section_filter->filterhandle[i], vs_demux_handle, pids[0], tableids[i]);
        section_filter->tableids[i] = tableids[i];
    }

    section_filter->pid_num     = pidnum;
    section_filter->tableid_num = tabldidnum;

    return 0;
}

static int32_t demux_stop_section(filter_t *filter)
{
    int32_t i;
    section_filter_t *section_filter = NULL;

    section_filter = &filter->u.section_filter;

    printf("demux_stop_section filter %p, demux_handle %d !\n", filter, (int)filter->vs_demux_handle);

    for(i = 0; i < section_filter->tableid_num; i++)
    {
        Thal_Demux_ReleaseSecFilter(filter->vs_demux_handle, section_filter->filterhandle[i]);
        printf("demux_stop_section table_id %d  !\n", section_filter->tableids[i]);
    }

    return 0;
}

static int32_t demux_start_record(filter_t *filter, int32_t upid[], uint8_t pidnum)
{
    tridDmxRecorderCfg_t recordconfig;
    int32_t i, ret;
    pthread_t *thread_id;

    recordconfig.pidnum = pidnum;
    for (i = 0; i < pidnum; i++) {
        recordconfig.pid[i] = upid[i];
        printf("demux_start_record, index %d ,pid 0x%x ,num %d !\n",i ,upid[i], pidnum);
    }

    /* use by playback, not use and disable */
    recordconfig.btimestamp = 0;
    recordconfig.bouteventlog = 0;
    /* recordmsginterval mean when 512 * 16 ts package come and demux would interrupt to nofity
     * but polling doesn't need interrupt, 512 * 16 is big enought to stop interrupt happen
     */
    recordconfig.recordmsginterval = 512 * 16;// change according to bitrate later
    ret = Thal_Demux_SetRecordCfg(filter->vs_demux_handle, &recordconfig);
    if (ret) {
        printf("Thal_Demux_SetRecordCfg failed\n");
        return -1;
    }

    ret = Thal_Demux_EnableRecord(filter->vs_demux_handle, trid_true);
    if (ret) {
        printf("Thal_Demux_EnableRecord failed\n");
        return -1;
    }

    thread_id = &filter->u.ts_filter.thread_id;
    ret = pthread_create(thread_id, NULL, recorder_data_thread, (void *)filter);
    if (ret) {
        printf("%s create recorder_data_thread thread fail ret=%d\n", __func__, ret);
        return -1;
    }

#if SAVE_DEMUX_STREAM
    demux_record_file = fopen("/sdcard/demux_record.ts", "wb+");
    if (!demux_record_file) {
        printf("open /sdcard/demux_record.ts fail\n\n");
    }
#endif

    return 0;
}

static int32_t demux_stop_record(filter_t *filter)
{
    int32_t ret;
    pthread_t thread_id = 0;

    thread_id = filter->u.ts_filter.thread_id;
    if (thread_id != 0)
        ret = pthread_join(thread_id, NULL);

    printf("demux_stop_record filter %p, demux_handle %d !\n", filter, (int)filter->vs_demux_handle);

    ret = Thal_Demux_StopRecord(filter->vs_demux_handle);
    if (ret) {
        printf("%s Thal_Demux_StopRecord failed ret=%d\n", __func__, ret);
        return -1;
    }

#if SAVE_DEMUX_STREAM
    if(demux_record_file)
    {
         fclose(demux_record_file);
         demux_record_file = NULL;
    }
#endif

    return 0;
}

static int32_t demux_start_allTS(filter_t *filter)
{
    int ret;
    pthread_t *thread_id;
    allts_filter_t *allts_filter;

    allts_filter = &filter->u.allts_filter;
    thread_id = &filter->u.allts_filter.thread_id;
    ret = pthread_create(thread_id, NULL, AllTsProcess, (void *)filter);
    if (ret) {
        printf("%s create AllTsProcess thread fail ret=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int32_t demux_stop_allTS(filter_t *filter)
{
    int32_t ret = 0;
    pthread_t thread_id = 0;

    thread_id = filter->u.allts_filter.thread_id;
    if (thread_id != 0)
        ret = pthread_join(thread_id, NULL);

    if (ret) {
        printf("join AllTsProcess thread fail ret=%d\n", ret);
        return -1;
    }

#if SAVE_DEMUX_STREAM
    if(demux_cip_file)
    {
         fclose(demux_cip_file);
         demux_cip_file = NULL;
    }
#endif

    return 0;
}

static int32_t demux_build_alltsdatapath(filter_t *filter, tridCIPInputSource_e tssource)
{
    int32_t ret, dma_buf_fd;
    allts_filter_t *allts_filter;
    tridDataPathHandle_t handle = DMX_INVALID_HANDLE;
    tridCIPDataPathCfg_t stCIPDataPathCfg;
    tridDataPathHandle_t demuxdatapathhandle;

    allts_filter = &filter->u.allts_filter;
    allts_filter->imemsize = CIP_BUFFERSIZE;

#if TVFE_IOMMU_ENABLE
    printf("DTMB IOMMU enable\n");
    unsigned char *addr_vir = NULL;
    unsigned long addr_phy = 0;

    ret = dtv_ion_open();
    if (ret) {
        printf("%s dtv_ion_open fail=%d\n", __func__, ret);
        return -1;
    }

    ret = dtv_ion_AllocFd(CIP_BUFFERSIZE, &dma_buf_fd);
    if (ret) {
        printf("%s dtv_ion_AllocFd fail ret=%d\n", __func__, ret);
        return -1;
    }

    ret = dtv_ion_mmap(dma_buf_fd, CIP_BUFFERSIZE, &addr_vir);
    if (ret < 0) {
        printf("dtv_ion_mmap error\n\n");
        return -1;
    }

    ret = dtv_ion_get_phyAddr(dma_buf_fd, &addr_phy);
    if(ret < 0) {
        printf("get phy addr error\n\n");
        dtv_ion_munmap(CIP_BUFFERSIZE, addr_vir);
        return -1;
    }
    allts_filter->dma_buf_fd = dma_buf_fd;
    allts_filter->pbuffer_phy = (unsigned int)addr_phy;
    allts_filter->pbuffer_vir = (void *)addr_vir;
#else
    allts_filter->pbuffer_vir = Trid_Util_CPUComm_Malloc(allts_filter->imemsize);
    if (allts_filter->pbuffer_vir == NULL)
    {
        printf("%s Trid_Util_CPUComm_Malloc fail, request memsize:0x%x", __func__, allts_filter->imemsize);
        return -1;
    }
    allts_filter->pbuffer_phy = (unsigned int)Trid_Util_CPUComm_Convert2PhyAddr(allts_filter->pbuffer_vir);
#endif
    printf("AllTS CIP Buf phyaddr=0x%x vir=%p\n", allts_filter->pbuffer_phy,
                                                 allts_filter->pbuffer_vir);

    stCIPDataPathCfg.tssource = tssource;
    stCIPDataPathCfg.tsmode = CIP_TS_MODE_CAPTURE;
    stCIPDataPathCfg.cipath = CIP_CI_BYPASS;
    stCIPDataPathCfg.capbufaddr = allts_filter->pbuffer_phy;
    stCIPDataPathCfg.capbuflen = allts_filter->imemsize;
    ret = Thal_CIP_ReqDataPath(&handle, &demuxdatapathhandle, &stCIPDataPathCfg);
    if (ret) {
        printf("%s Thal_CIP_ReqDataPath fail=%d\n", __func__, ret);
        return -1;
    }

    ret = Thal_CIP_EnableDataPath(handle, trid_true);
    if (ret) {
        printf("%s Thal_CIP_EnableDataPath fail=%d\n", __func__, ret);
        return -1;
    }

    filter->vs_demux_handle = handle;

    return 0;
}

static int32_t demux_release_alltsdatapath(filter_t *filter)
{
    int32_t ret;
    allts_filter_t *allts_filter;

    allts_filter = &filter->u.allts_filter;

#if TVFE_IOMMU_ENABLE
    //Release CIP path firstly and later to free memory
    ret = Thal_CIP_ReleaseDataPath(filter->vs_demux_handle);
    if (ret)
    {
        printf("%s Thal_CIP_ReleaseDataPath fail ret=%d\n", __func__, ret);
        return -1;
    }

    ret = dtv_ion_FreeFd(allts_filter->dma_buf_fd);
    if (ret)
    {
        printf("%s dtv_ion_munmap fail ret=%d\n", __func__, ret);
        return -1;
    }

    ret = dtv_ion_munmap(allts_filter->imemsize, allts_filter->pbuffer_vir);
    if (ret)
    {
        printf("%s dtv_ion_munmap fail ret=%d\n", __func__, ret);
        return -1;
    }

    dtv_ion_CloseFd(allts_filter->dma_buf_fd);

	ret = dtv_ion_close();
    if (ret)
    {
        printf("%s dtv_ion_close fail ret=%d\n", __func__, ret);
        return -1;
    }
#else
    ret = Trid_Util_CPUComm_Free(allts_filter->pbuffer_vir);
    if (ret) {
        printf("%s Trid_Util_CPUComm_Free fail ret=%d\n", __func__, ret);
        return -1;
    }

    ret = Thal_CIP_ReleaseDataPath(filter->vs_demux_handle);
    if (ret) {
        printf("%s Thal_CIP_ReleaseDataPath fail ret=%d\n", __func__, ret);
        return -1;
    }
#endif

    return 0;
}

static int32_t demux_build_datapath(tridDataPathHandle_t *phandle, tridDmxInputSource_e tssource)
{
    tridDataPathHandle_t handle = DMX_INVALID_HANDLE;
    unsigned int cookie = 0;//this value is be ignored
    tridDataPathCfg_t stDataPathCfg;
    tridDmxEvent_e event;
    int32_t ret;

    if(dmx_mgr.path_handle != DMX_INVALID_HANDLE)
    {
        pthread_mutex_lock(&dmx_mgr.mutex);
        dmx_mgr.path_ref++;
        *phandle = dmx_mgr.path_handle;
        pthread_mutex_unlock(&dmx_mgr.mutex);

        printf("%s datapath handle %d ref = %d.", __func__, (int)dmx_mgr.path_handle, dmx_mgr.path_ref);
        return 0;
    }

    ret = Thal_Demux_ReqDataPath(&handle, tssource, cookie);
    if(ret) {
        printf("%s Thal_Demux_ReqDataPath fail ret=%d\n", __func__, ret);
        return -1;
    }

    stDataPathCfg.tsmode = TS_MODE_DVB;
    stDataPathCfg.cipath = CI_Bypass;

#if 0
    ret = Thal_Demux_SetDataPathCfg(handle, &stDataPathCfg);/*this interface only used by CI*/
    if(ret) {
        printf("%s Thal_Demux_SetDataPathCfg failed", __func__);
        //return -1; //TODO*
    }
#endif

    ret = Thal_Demux_EnableDataPath(handle, 1);
    if(ret) {
        printf("%s Thal_Demux_EnableDataPath failed", __func__);
        return -1;
    }

    pthread_mutex_lock(&dmx_mgr.mutex);
    dmx_mgr.path_handle = handle;
    dmx_mgr.path_ref++;
    pthread_mutex_unlock(&dmx_mgr.mutex);

    *phandle = handle;

    return 0;
}

static int32_t demux_release_datapath(tridDataPathHandle_t vs_demux_handle, demux_filter_type_t type)
{
    int32_t ret;

    printf("demux_release_datapath demux_handle %d, path_ref %d!\n", (int)vs_demux_handle, dmx_mgr.path_ref);

    pthread_mutex_lock(&dmx_mgr.mutex);
    if(--dmx_mgr.path_ref <= 0)
    {
        if (type != DMX_FILTER_TYPE_ALLTSPASS)
        {
            ret = Thal_Demux_ReleaseDataPath(vs_demux_handle);
            if (ret) {
                printf("%s Thal_Demux_ReleaseDataPath fail=%d\n", __func__, ret);
                return -1;
            }
        }
        dmx_mgr.path_ref    = 0;
        dmx_mgr.path_handle = DMX_INVALID_HANDLE;
    }
    pthread_mutex_unlock(&dmx_mgr.mutex);

    return 0;
}

static int32_t section_main_task(filter_t *filter, demux_filter_param_t *filter_param)
{
    int32_t ret;
    tridDmxInputSource_e tssource;

    tssource = get_input_source_type(filter);
    filter->vs_demux_handle = DMX_INVALID_HANDLE;
    ret = demux_build_datapath(&filter->vs_demux_handle, tssource);
    if (ret) {
        printf("%s demux_build_datapath fail=%d\n", __func__, ret);
        return -1;
    }
    printf("section_main_task filter %p, demux_handle %d !\n", filter, (int)filter->vs_demux_handle);

    ret = demux_start_section(&filter->u.section_filter, filter->vs_demux_handle, filter_param);
    if (ret) {
        printf("%s demux_start_section fail=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int32_t alltspass_main_task(filter_t *filter, demux_filter_param_t *filter_param)
{
    int32_t ret;
    tridCIPInputSource_e tssource;

    tssource = get_input_source_type(filter);
    filter->vs_demux_handle = DMX_INVALID_HANDLE;
    ret = demux_build_alltsdatapath(filter, tssource);
    if (ret) {
        printf("%s demux_build_alltsdatapath fail=%d\n", __func__, ret);
        return -1;
    }

#if SAVE_DEMUX_STREAM
    demux_cip_file = fopen("/sdcard/demux_cip.ts", "wb+");
    if (!demux_cip_file) {
        printf("open /sdcard/demux_cip.ts fail\n");
    }
#endif

    ret = demux_start_allTS(filter);
    if (ret) {
        printf("%s demux_start_allTS fail=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int32_t ts_main_task(filter_t *filter, demux_filter_param_t *filter_param)
{
    int32_t ret;
    int32_t *upid;
    uint8_t pidnum;
    tridDmxInputSource_e tssource;

    pidnum = filter_param->pid_num;
    upid = filter_param->pids;
    tssource = get_input_source_type(filter);
    filter->vs_demux_handle = DMX_INVALID_HANDLE;

    ret = demux_build_datapath(&filter->vs_demux_handle, tssource);
    if (ret) {
        printf("%s demux_build_datapath fail=%d\n", __func__, ret);
        return -1;
    }

    printf("ts_main_task filter %p, demux_handle %d !\n", filter, (int)filter->vs_demux_handle);
    ret = demux_start_record(filter, upid, pidnum);
    if (ret) {
        printf("%s demux_start_record fail=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int32_t allocate_filter(demux_filter_param_t *filter_param, mpegts_context_t *ctx)
{
    filter_t *filter;
    ts_filter_t *ts;
    section_filter_t *section;
    allts_filter_t *allts_filter;

    pthread_mutex_lock(&ctx->mutex);

    filter = (filter_t *)malloc(sizeof(filter_t));
    if (NULL == filter) {
        pthread_mutex_unlock(&ctx->mutex);
        return -1;
    }
    memset(filter, 0, sizeof(filter_t));

    /* for TS and section, we start immediately. */
    filter->status = DEMUX_STATUS_STARTED;
    if (filter_param->filter_type == DMX_FILTER_TYPE_TS) {
        ts = &filter->u.ts_filter;
        ts->requestbufcb = filter_param->request_buffer_cb;
        ts->updatedatacb = filter_param->update_data_cb;
        ts->push_ts_data = (push_data_cb)push_ts_data;
        ts->cookie = filter_param->cookie;// save private data
    } else if (filter_param->filter_type == DMX_FILTER_TYPE_SECTION) {
        section = &filter->u.section_filter;
        section->last_cc = -1;
        section->requestbufcb = filter_param->request_buffer_cb;
        section->updatedatacb = filter_param->update_data_cb;
        section->push_section_data = (push_data_cb)push_section_data;
        section->cookie = filter_param->cookie;// save private data
    } else if (filter_param->filter_type == DMX_FILTER_TYPE_ALLTSPASS) {
        allts_filter = &filter->u.allts_filter;
        allts_filter->requestbufcb = filter_param->request_buffer_cb;
        allts_filter->updatedatacb = filter_param->update_data_cb;
        allts_filter->push_allts_data = (push_data_cb)push_allts_data;
        allts_filter->cookie = filter_param->cookie;// save private data
        allts_filter->data_offset = 0;
        allts_filter->frame_complete = TRUE;
    } else {
        printf("invalid operation...filter_type only support AllTSPASS/SECTION/TS\n\n");
        free(filter);
        return -1;
    }

    pthread_mutex_init(&filter->mutex, NULL);
    /* save filter type into mpegts instance */
    filter->type = filter_param->filter_type;
    ctx->filter[filter->type] = filter;
    printf("ts_vs allocate  filter finish, filter addr %p, type %d !\n", filter, filter->type);

    pthread_mutex_unlock(&ctx->mutex);

    return 0;
}

static int32_t free_filter(mpegts_context_t *ctx, int filter_type)
{
    filter_t* filter;

    pthread_mutex_lock(&ctx->mutex);
    filter = ctx->filter[filter_type];
    if (filter == NULL) {
        printf("%s mpegts_context->filter is NULL, free fail", __func__);
        pthread_mutex_unlock(&ctx->mutex);
        return -1;
    }
    printf("tsdemux free filter %p, type %d !\n", filter, filter_type);
    pthread_mutex_destroy(&filter->mutex);
    free(filter);
    ctx->filter[filter_type] = NULL;

    pthread_mutex_unlock(&ctx->mutex);

    return 0;
}

static int32_t open_filter(demux_filter_param_t *filter_param, mpegts_context_t *ctx)
{
    int32_t ret;
    filter_t* filter;
    tridDmxInputSource_e ts_input_source;

    ret = allocate_filter(filter_param, ctx);
    if (ret < 0) {
        printf("%s allocate filter fail ret=%d\n", __func__, ret);
        return -1;
    }

    filter = ctx->filter[filter_param->filter_type];
    switch (filter->type) {
    case DMX_FILTER_TYPE_TS:
        ret = ts_main_task(filter, filter_param);
        if (ret) {
            printf("%s ts_main_task fail ret=%d\n", __func__, ret);
            return -1;
        }
        break;

    case DMX_FILTER_TYPE_ALLTSPASS:
        ret = alltspass_main_task(filter, filter_param);
        if (ret) {
            printf("%s alltspass_main_task fail ret=%d\n", __func__, ret);
            return -1;
        }
        break;

    case DMX_FILTER_TYPE_SECTION:
        ret = section_main_task(filter, filter_param);
        if (ret) {
            printf("%s section_main_task fail ret=%d\n", __func__, ret);
            return -1;
        }
        break;

    default:
        printf("%s invalid operation...filter overtype", __func__);
        filter->status = DEMUX_STATUS_STOPPED;
        ret = free_filter(ctx, filter->type);
        if (ret)
            printf("%s free_filter fail ret=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int32_t close_filter(mpegts_context_t *ctx, int32_t filter_type)
{
    int32_t ret;
    filter_t *filter;

    filter = ctx->filter[filter_type];
    if (filter == NULL)
    {
        printf("%s filter is NULL, free fail", __func__);
        return -1;
    }

    filter->status = DEMUX_STATUS_STOPPED;
    switch (filter->type) {
    case DMX_FILTER_TYPE_TS:
        ret = demux_stop_record(filter);
        if (ret) {
            printf("%s demux_stop_record fail ret=%d\n", __func__, ret);
            return -1;
        }
        break;

    case DMX_FILTER_TYPE_ALLTSPASS:
        ret = demux_stop_allTS(filter);
        if (ret) {
            printf("%s demux_stop_allTS fail ret=%d\n", __func__, ret);
            return -1;
        }

        ret = demux_release_alltsdatapath(filter);
        if (ret) {
            printf("%s demux_release_alltsdatapath fail ret=%d\n", __func__, ret);
            return -1;
        }
        break;

    case DMX_FILTER_TYPE_SECTION:
        ret = demux_stop_section(filter);
        if (ret) {
            printf("%s demux_stop_section fail ret=%d\n", __func__, ret);
            return -1;
        }
        break;

    default:
        printf("%s invalid operation...", __func__);
        return -1;
    }

    ret = demux_release_datapath(filter->vs_demux_handle, filter->type);
    if (ret) {
        printf("%s demux_release_datapath fail ret=%d\n", __func__, ret);
        return -1;
    }

    ret = free_filter(ctx, filter->type);
    if (ret) {
        printf("%s free_filter fail ret=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

static void *ts_demux_vs_open(void)
{
    int ret, i;
    mpegts_context_t *mp;
    tridDmxCfg_t stDmxCfg;

    mp = (mpegts_context_t *)malloc(sizeof(mpegts_context_t));
    if (mp == NULL) {
        printf("%s malloc mpegts_context fail", __func__);
        return NULL;
    }

    memset(mp, 0, sizeof(mpegts_context_t));
    pthread_mutex_init(&mp->mutex, NULL);

    for(i = 0; i < MAX_FILTER_NUM; i++)
        mp->filter[i] = NULL;

#if !TVFE_IOMMU_ENABLE
    ret = Trid_Util_CPUComm_Init(0);
    if (ret) {
        printf("%s Trid_Util_CPUComm_Init error", __func__);
        if(mp)
        {
            pthread_mutex_destroy(&mp->mutex);
            free(mp);
        }
        return NULL;
    }
    printf("Trid_Util_CPUComm_Init end !\n");
#endif

    /* demux-cip module inital */
    Thal_CIP_Init();

    dmx_mgr.path_ref    = 0;
    dmx_mgr.path_handle = DMX_INVALID_HANDLE;

#if ENABLE_FILTER_MULTIPLE
    pthread_mutex_init(&allts_mutex_ext, NULL);
    reset_ext_filter(&allts_filter_ext);
#endif

    printf("Thal_Demux_Init end !\n");

    return (void *)mp;
}

static int32_t ts_demux_vs_close(void *handle)
{
    int32_t i = 0;
    mpegts_context_t *mp = (mpegts_context_t *)handle;

    if(mp == NULL)
        return -1;

    printf("%s line %d,dmx_mng.dmx_ref = %d.", __func__, __LINE__, dmx_mgr.ref);

    for (i = 0; i < MAX_FILTER_NUM; i++)
    {
        if (mp->filter[i])
        {
            close_filter(mp, mp->filter[i]->type);
            mp->filter[i] = NULL;
        }
    }

    /* Exit demux driver */
    Thal_CIP_Exit();
    printf("Thal_CIP_Exit end !\n");
    pthread_mutex_destroy(&mp->mutex);

#if ENABLE_FILTER_MULTIPLE
    pthread_mutex_destroy(&allts_mutex_ext);
#endif

    free(mp);

    return 0;
}

static int32_t demux_GetBufferUsed(filter_t *filter)
{
    unsigned long usedbytes = 0;
    int32_t ret;

    ret = Thal_Demux_GetBufferUsed(filter->vs_demux_handle, DATA_TYPE_TS, &usedbytes);
    if(!ret) {
        /* the size of one ts frame is 188 byte */
        return (usedbytes / 188);
    } else {
        return -1;
    }
}

int32_t ts_demux_vs_open_filter(void *handle, demux_filter_param_t *filter_param)
{
    mpegts_context_t *mp = NULL;
    int32_t ret;

    mp = (mpegts_context_t *) handle;

    if (mp->filter[filter_param->filter_type]){
        /* filter aready exists */
        printf("ts_vs %d filter have already been open !\n", filter_param->filter_type);
        return -1;
    }

#if ENABLE_FILTER_MULTIPLE
    if(filter_param->filter_type == DMX_FILTER_TYPE_MULTIPLE)
    {
        pthread_mutex_lock(&allts_mutex_ext);
        allts_filter_ext.cookie           = filter_param->cookie;
        allts_filter_ext.requestbufcb     = filter_param->request_buffer_cb;
        allts_filter_ext.updatedatacb     = filter_param->update_data_cb;
        allts_filter_ext.push_allts_data  = (push_data_cb)push_allts_data;
        allts_filter_ext.data_offset = 0;
        allts_filter_ext.frame_complete = TRUE;
        pthread_mutex_unlock(&allts_mutex_ext);
        printf("tsdemux open multiple filter finish !\n");
        return 0;
    }
#endif

    ret = open_filter(filter_param, mp);
    if (ret) {
        printf("%s open filter fail ret=%d\n", __func__, ret);
        return -1;
    }

    return 0;
}

int32_t ts_demux_vs_close_filter(void* handle, int32_t filter_type)
{
    int32_t ret;
    mpegts_context_t *mp;

    mp = (mpegts_context_t *) handle;

#if ENABLE_FILTER_MULTIPLE
    if(filter_type == DMX_FILTER_TYPE_MULTIPLE)
    {
        reset_ext_filter(&allts_filter_ext);
        printf("tsdemux close multiple filter finish !\n");
        return 0;
    }
#endif

    ret = close_filter(mp, filter_type);
    if (ret) {
        printf("%s close filter fail ret=%d\n", __func__, ret);
        return -1;
    }

#if ENABLE_FILTER_MULTIPLE
    reset_ext_filter(&allts_filter_ext);
#endif

    printf("tsdemux close filter type %d finish !\n", filter_type);
    return 0;
}

void TSDMX_vs_setInputSource(demux_input_source_t input_source)
{
    dmx_mgr.input_source = input_source;
    printf("ts demux set input source type %d !\n", dmx_mgr.input_source);
}

void *TSDMX_vs_SingletonGetInstance(void)
{
    pthread_mutex_lock(&dmx_mgr.mutex);

    if (dmx_mgr.handle == NULL) {
        /* just support one instance */
        dmx_mgr.handle = ts_demux_vs_open();
    }
    dmx_mgr.ref++;
    printf("%s dmx_mgr.ref = %d.\n", __func__, dmx_mgr.ref);

    pthread_mutex_unlock(&dmx_mgr.mutex);
    return dmx_mgr.handle;
}

int32_t TSDMX_vs_Destroy(void *handle)
{
    pthread_mutex_lock(&dmx_mgr.mutex);
    if(--dmx_mgr.ref == 0) {
        ts_demux_vs_close(handle);
        dmx_mgr.handle = NULL;
        dmx_mgr.input_source = DMX_INPUT_SOURCE_UNKNOW;
    }
    pthread_mutex_unlock(&dmx_mgr.mutex);
    return 0;
}

int32_t ts_demux_vs_clear(void *handle)
{
    /* do nothing here */
    printf("ts_demux_clear\n");
    return 0;
}

int32_t ts_demux_vs_start(void *handle)
{
    /* do nothing here */
    printf("ts_demux_start\n");
    return 0;
}

int32_t ts_demux_vs_pause(void *handle)
{
    /* do nothing here */
    printf("ts_demux_pause\n");
    return 0;
}

int32_t ts_demux_vs_get_free_filter_num(void *handle)
{
    mpegts_context_t *mp = (mpegts_context_t *) handle;

#if 0
    if(mp) {
        if(mp->filter) {
            /* vs record feature only support one object */
            return 0;
        } else {
            /* filter have not been used */
            return 1;
        }
    }
#endif
    return 16;
}

int32_t ts_demux_vs_get_ts_packet_num(void *handle, int32_t filter_type) {
    mpegts_context_t *mp = (mpegts_context_t*) handle;
    int32_t packet_num = 0;

    if(mp) {
        if (mp->filter[filter_type]) {
            packet_num = demux_GetBufferUsed(mp->filter[filter_type]);
        }
    }

    return packet_num;
}

int32_t ts_demux_vs_set_buffer_size(void *handle, int size)
{
    printf("ts_demux_set_buffer_size\n");
    mpegts_context_t *mp = (mpegts_context_t *) handle;

    if(mp == NULL) {
        printf("invalid handle\n");
        return -1;
    }

    return 0;
}

int32_t ts_demux_vs_write_data(void *handle, void *buf, int size)
{
    mpegts_context_t *mp = (mpegts_context_t *) handle;

    if(mp == NULL) {
        printf("invalid handle\n");
        return -1;
    }

    return 0;
}
