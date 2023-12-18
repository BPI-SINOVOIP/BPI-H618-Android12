#ifndef DTV_TSDEMUX_I_VS_H
#define DTV_TSDEMUX_I_VS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "thal_demux_api.h"
#include <pthread.h>
#include "DataType.h"
#define MAX_SECTION_SIZE        4096
#define MAX_FILTER_NUM          DMX_FILTER_TYPE_END

//* define callback function to push data
typedef int32_t (*push_data_cb)(uint8_t* data, uint32_t len,
								uint32_t new_frm, void * parm);

enum DEMUX_STATUS {
	DEMUX_STATUS_IDLE = 0,
	DEMUX_STATUS_STARTED,
	DEMUX_STATUS_STOPPED,
	DEMUX_STATUS_PAUSED,
};

typedef struct ALLTSFILTER {
	pthread_t thread_id;
	unsigned int pbuffer_phy;
	void *pbuffer_vir;
	size_t imemsize;
	int32_t dma_buf_fd; //save dma buffer fd which is used for CIP

	uint8_t *cur_ptr; //* current pointer
	BOOL frame_complete;  //* TRUE mean complete, FALSE mean no
	uint32_t free_size; //* free size of buffer for pushing data
	uint32_t data_offset; //* save cur_ptr pointer offset
	md_buf_t md_buf;

	pdemux_callback_t requestbufcb;
	pdemux_callback_t updatedatacb;
	push_data_cb push_allts_data; //* push TS data callback function
	void *cookie;
} allts_filter_t;

typedef struct TSFILTER {
	pthread_t thread_id;
	uint8_t *cur_ptr; //* current pointer
	uint8_t pids_num;
	uint32_t free_size; //* free size of buffer for pushing data
	int32_t pids[DMX_PVR_MAX_PID_NUM];

	md_buf_t md_buf;
	pdemux_callback_t requestbufcb;
	pdemux_callback_t updatedatacb;
	push_data_cb push_ts_data; //* push TS data callback function
	void *cookie;
} ts_filter_t;

typedef struct SECTIONFILTER {
	uint32_t need_start;
	int32_t section_index;
	int32_t section_h_size;

	int32_t check_crc;
	int32_t end_of_section_reached;
	int32_t last_cc; //continuity counter

	uint8_t *cur_ptr;
	uint32_t free_size;
	uint8_t pid_num;
	uint8_t tableid_num;
	//int32_t pids[DMX_PVR_MAX_PID_NUM];
	int32_t tableids[DMX_FILTER_SIZE];
	tridFilterHandle_t filterhandle[DMX_FILTER_SIZE];

	md_buf_t md_buf;
	pdemux_callback_t requestbufcb; //request buffer callback function
	pdemux_callback_t updatedatacb; //update data callback function
	push_data_cb push_section_data; //push section data callback function
	void *cookie;
} section_filter_t;

//* define MPEG TS filter structure
typedef struct MPEGTSFILTER {
	tridDataPathHandle_t vs_demux_handle;//handle for VS Demux module
	demux_filter_type_t type;
	pthread_mutex_t mutex;
	int32_t status;
	union {
		ts_filter_t ts_filter;
		section_filter_t section_filter;
		allts_filter_t allts_filter;
	} u;
} filter_t;

typedef struct MPEGTSCONTEXT {
	pthread_mutex_t mutex;
	filter_t *filter[MAX_FILTER_NUM];
} mpegts_context_t;

#ifdef __cplusplus
}
#endif

#endif

