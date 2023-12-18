/**@file
*    @brief		The head file of DMX module
*    @author	xhw
*    @date		2015-10-10
*    @version		1.0.0
*    @note		Copyright(C) Allwinner Corporation. All rights reserved.
*/

/****************************INCLUDE FILE************************************/
#ifndef DEMUX_WRAPPER_H_
#define DEMUX_WRAPPER_H_

#include "DVBCoreEpg.h"

//#define  DMX_DUBUG
#ifdef __cplusplus
extern "C" {
#endif
//****************************************************************************//
//* Demux.
//****************************************************************************//
typedef struct filter_info 
{
	unsigned char* buffer;
	int buf_size;
	TSParser* mTSParser;
	int pid;
	int service_id;
	int nChanId;
	int type;
} filter_info;

/****************************************************************************
* Function Name : DVBCoreDemuxOpenFilter
* Description : Open section Filter
* Parameters :
* demux_handle -- the demux device handle
* filter -- the filter info
* Returns : -1 - failure 0 - success
******************************************************************************/
int DVBCoreDemuxOpenFilter(int *demux_handle, filter_info *filter);

/****************************************************************************
* Function Name : DemuxWriteData
* Description : DVBCoreDemuxCloseFilter
* Parameters :
* demux_handle -- the demux device handle
* pid -- 
* nChanId -- 
* Returns : -1 - failure 0 - success
******************************************************************************/
int DVBCoreDemuxCloseFilter(int *demux_handle, int pid, int nChanId);

#ifdef __cplusplus
}
#endif

#endif /*DEMUX_WRAPPER_H_ */
