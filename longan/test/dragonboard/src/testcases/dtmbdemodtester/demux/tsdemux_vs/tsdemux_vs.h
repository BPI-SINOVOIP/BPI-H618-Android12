#ifndef DTV_TSDEMUX_VS_H
#define DTV_TSDEMUX_VS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SAVE_DEMUX_STREAM      (0)
#define ENABLE_FILTER_MULTIPLE (1)
#define TVFE_IOMMU_ENABLE      (1)

#if ENABLE_FILTER_MULTIPLE
#define SP_FILTER_TYPE  DMX_FILTER_TYPE_ALLTSPASS
#define EIT_FILTER_TYPE DMX_FILTER_TYPE_MULTIPLE
#else
#define SP_FILTER_TYPE  DMX_FILTER_TYPE_TS
#define EIT_FILTER_TYPE DMX_FILTER_TYPE_SECTION
#endif

//define callback function
typedef int (*pdemux_callback_t)(void* param, void* cookie);

typedef enum DEMUX_STREAM_TYPE
{
    DMX_STREAM_UNKOWN = 0,
    DMX_STREAM_VIDEO,
    DMX_STREAM_AUDIO,
    DMX_STREAM_SUBTITLE,
    DMX_STREAM_SECTION,
}demux_stream_type;

typedef enum DEMUX_FILTER_TYPE
{
    DMX_FILTER_TYPE_ES = 0,     //* get es stream from a filter.
    DMX_FILTER_TYPE_PES,        //* get pes packets from a filter.
    DMX_FILTER_TYPE_TS,         //* get special pid ts packets from a filter.
    DMX_FILTER_TYPE_SECTION,    //* get si section from a filter.
    DMX_FILTER_TYPE_RECORD,     //* get special pid record(ts packets) from a filter.
    DMX_FILTER_TYPE_ALLTSPASS,    //* get all ts packets from a filter
    DMX_FILTER_TYPE_MULTIPLE,
    DMX_FILTER_TYPE_END,
} demux_filter_type_t;

typedef enum DEMUX_INPUT_SOURCE
{
    DMX_INPUT_SOURCE_UNKNOW = 0,
    DMX_INPUT_SOURCE_ATSC_DVB,  // atsc/dvb input
    DMX_INPUT_SOURCE_DTMB,      // dtmb input
}demux_input_source_t;

typedef struct MEDIA_BUFFER
{
    unsigned char*  buf;
    unsigned int    buf_size;
}md_buf_t;

typedef struct DEMUX_FILTER_PARAM
{
    int32_t    stream_type;
    int32_t    codec_type;
    int32_t    nChanId;
    demux_filter_type_t filter_type;
    pdemux_callback_t   request_buffer_cb;
    pdemux_callback_t   update_data_cb;
    void*               cookie;

    int32_t* pids;
    uint8_t* tableids;
    uint8_t pid_num;
    uint8_t tableid_num;
} demux_filter_param_t;

void TSDMX_vs_setInputSource(demux_input_source_t input_source);//extend_ionterface
void *TSDMX_vs_SingletonGetInstance(void);
int32_t TSDMX_vs_Destroy(void* handle);

/****************************************************************************
* Function Name : ts_demux_open_filter
* Description : open a demux device filter
* Parameters :
* handle    -- the demux device handle
* param        -- the demux filter parameters
* Returns    -- -1 - failure 0 - success
******************************************************************************/
int32_t ts_demux_vs_open_filter(void *handle, demux_filter_param_t *filter_param);

/****************************************************************************
* Function Name : ts_demux_close_filter
* Description : close a demux device filter
* Parameters :
* handle -- the demux device handle
* Returns : -1 - failure 0 - success
******************************************************************************/
int32_t ts_demux_vs_close_filter(void* handle, int32_t filter_type);

int32_t  ts_demux_vs_clear(void *handle);
int32_t  ts_demux_vs_start(void *handle);
int32_t  ts_demux_vs_pause(void *handle);

/****************************************************************************
* Function Name : ts_demux_get_free_filter_num
* Description : get free filter number
* Parameters :
* handle -- the demux device handle
* Returns : -1 - failure 0 - filter resource busy 1 - filter idle
******************************************************************************/
int32_t  ts_demux_vs_get_free_filter_num(void* handle);
int32_t  ts_demux_vs_get_ts_packet_num(void* handle, int32_t filter_type);

//for injector.
int32_t  ts_demux_vs_set_buffer_size(void *handle, int size);
int32_t  ts_demux_vs_write_data(void *handle, void *buf, int size);

#ifdef __cplusplus
}
#endif
#endif

