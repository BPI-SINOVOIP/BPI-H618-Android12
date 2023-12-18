/**@file
*    @brief		The head file of Program Manager module
*    @author	xhw
*    @date		2015-10-12
*    @version		1.0.0
*    @note		Copyright(C) Allwinner Corporation. All rights reserved.
*/
/**************************************************************************/

#ifndef PROGRAM__MANAGER_CONTEXT_H
#define PROGRAM__MANAGER_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include "DVBCoreEpg.h"
#define DVBCORE_PM_SUCCESS 0
#define DVBCORE_PM_FAILURE -1

typedef int (*PMCallbackInternal)(void* pUserData, int eMessageId, void* param);
typedef void (*EPGCallbackInternal)(int status, int event_count, void *param);

enum EPMNOTIFY  //* internal notify.
{
    PM_CMD_GET_PROGRAM       = 100,
	PM_CMD_SCAN_END,
	PM_CMD_END,
	PM_CMD_GET_TIME,
	PM_CMD_GET_EPG
};

enum PMStatus
{
    DVBCORE_PM_INITIALIZED,
    DVBCORE_PM_IDLE,
	DVBCORE_PM_SEARCHING,    
    DVBCORE_PM_PAT_SEARCHED,
	DVBCORE_PM_PMT_SEARCHED,
	DVBCORE_PM_SDT_SEARCHED,
	DVBCORE_PM_SEARCHED,	
};

enum DVBCoreStatusE
{
	DVBCORE_ERR = -1,
	DVBCORE_OK,
    DVBCORE_INVALID,
    DVBCORE_MALLOC_FAIL,
    DVBCORE_SEARCH_FAIL,
    DVBCORE_INVALID_OPERATION,
    DVBCORE_UNKNOWN_ERR,
    /*other error info*/
};

typedef struct PROGRAM_MANAGER_CONTEXT
{

	void *			demux_handle;
	enum PMStatus		eStatus;
    pthread_mutex_t statusLock;
    pthread_cond_t 	cond;
    pthread_t       SearchThreadId;
	int				nHasSearchedNum;              	                            
	pthread_t       timeMonitorThreadId;
	int           	bTimeMonitorStopFlag;
	              	
	//pthread_t       epgMonitorThreadId;
	pthread_t       emmGetThreadId;
	//int           	bEpgMonitorStopFlag;
	int           	bEmmGetStopFlag;
	
	PMCallbackInternal	callback;
	void*			pUserData;
	TSParser*		mTSParser;
	int end;
	int mErrno;
	
} PMContext;

typedef struct EPG_CONTEXT
{
	void *			demux_handle;
    pthread_mutex_t statusLock;
    pthread_cond_t 	cond;          	                            
	pthread_t       epgMonitorThreadId;
	int           	bIsStarted;
	TSParser*		mTSParser;
	int 			nEventCount;
    int             mEpgSearchEnd;
    int             mSearchCount;
    unsigned int    mProgNum[32]; //add for EIT parse
    EPGCallbackInternal callback_i;

} EPGContext;

/*********************************************************************************************************************************
* Function Name : PMCompSetCallback
* Description : set callback for PM
* Parameters :
* callback -- 
* pUserData --
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int PMCompSetCallback(void *p, PMCallbackInternal callback, void* pUserData);

/*********************************************************************************************************************************
* Function Name : CreateProgramManager
* Description : create the struct of Program Manager context
* Parameters :
* Returns : the point of Program Manager context
*********************************************************************************************************************************/
PMContext* CreateProgramManager(void);

/*********************************************************************************************************************************
* Function Name : ProgramManagerReset
* Description : reset the struct of Program Manager context
* Parameters :
* pPMContext -- the point of Program Manager context
* Returns : void
*********************************************************************************************************************************/
int ProgramManagerReset(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : ProgramManagerDestroy
* Description : destroy the struct of Program Manager context
* Parameters :
* pPMContext -- the point of Program Manager context
* Returns : void
*********************************************************************************************************************************/
int ProgramManagerDestroy(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : ProgramSearchStart
* Description : start the Program Search thread
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ProgramSearchStart(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : ProgramSearchStop
* Description : stop the Program Search thread 
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int ProgramSearchStop(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : EpgMonitorStart
* Description : start the epg monitor thread
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int EpgMonitorStart(EPGContext* pEPGContext);

/*********************************************************************************************************************************
* Function Name : EpgMonitorStop
* Description : stop the epg Monitor thread  
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int EpgMonitorStop(EPGContext* pEPGContext);

/*********************************************************************************************************************************
* Function Name : TimeMonitorStart
* Description : start the time monitor thread
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int TimeMonitorStart(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : TimeMonitorStop
* Description : stop the Time Monitor thread 
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int TimeMonitorStop(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : EMMGetStart
* Description : start the EMM pid get thread
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int EMMGetStart(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : EMMGetStop
* Description : stop the EMM pid get thread
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int EMMGetStop(PMContext* pPMContext);

/*********************************************************************************************************************************
* Function Name : EpgManagerDestroy
* Description : 
* Parameters :
* arg -- 
* Returns : -1 - failure 0 - success
*********************************************************************************************************************************/
int EpgManagerDestroy(EPGContext* pEpgContext);

/*********************************************************************************************************************************
* Function Name : CreateEpgManager
* Description : 
* Parameters :
* arg -- 
* Returns : 
*********************************************************************************************************************************/
EPGContext* CreateEpgManager(void);

#ifdef __cplusplus
}
#endif

#endif  //PROGRAM__MANAGER_CONTEXT_H

