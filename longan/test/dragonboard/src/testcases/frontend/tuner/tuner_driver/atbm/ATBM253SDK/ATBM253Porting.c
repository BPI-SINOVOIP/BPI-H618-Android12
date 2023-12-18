/********************************************************************************************************
File:ATBM253Porting.c
Description:
    ATBM253 Tuner porting APIs which need to be implemented by user.

*********************************************************************************************************/
#include "ATBM253Porting.h"

#include <os_adapter.h>
#include "common_define.h"
#include "fe_log.h"

#if (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_USER_SPACE)
#include <pthread.h>
static pthread_mutex_t ATBM253Mutex = PTHREAD_MUTEX_INITIALIZER;
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_KERNEL_SPACE)
#include <linux/mutex.h>
static struct mutex ATBM253Mutex;
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_WINDOWS)
static HANDLE ATBM253Mutex;
#else
/*To realize mutex according to system platform.*/
#endif

/********************************************************************
* Function: ATBM253I2CRead
* Description: Read I2C data. User need to implement this function according to system platform.
*            Detail of ATBM253 I2C format is described in software guide document.
* Input:     pI2CAddr -- I2C slave address and user param
*        BaseReg -- Base register address
*        OffReg -- Offset register address
*        pData -- Pointer to register value
*        Length - Buffer length of 'pData' in byte
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253I2CRead(ATBM253I2CAddr_t *pI2CAddr,ATBM_U8 BaseReg,ATBM_U8 OffReg,ATBM_U8 *pData, ATBM_U32 Length)
{
    UINT8 ui8addrData[2] = {BaseReg, OffReg};
    UINT8 u8retry_times = 10, i;
    ATBM253_ERROR_e ret;
    TV_UNUSE(*pI2CAddr);

    for(i = 0; i < u8retry_times; i++)
    {
        ret = ReadRegWithDataLen_tv(ui8addrData, 2, (UINT8 *)pData, (UINT16)Length);
        if(ret == ATBM253_NO_ERROR) {
            break;
        } else {
            FE_LOGE("[%s] read regoff=0x%x ret=%d fail\n", __func__, OffReg, ret);
            ret = ATBM253_ERROR_I2C_FAILED;
        }
    }

    return ret;
}

/********************************************************************
* Function: ATBM253I2CWrite
* Description: Write I2C data. User need to implement this function according to system platform.
*            Detail of ATBM253 I2C format is described in software guide document.
* Input:     pI2CAddr -- I2C slave address and user param
*        BaseReg -- Base register address
*        OffReg -- Offset register address
*        pData -- Register value
*        Length - Buffer length of 'pData' in byte
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253I2CWrite(ATBM253I2CAddr_t *pI2CAddr,ATBM_U8 BaseReg,ATBM_U8 OffReg,ATBM_U8 *pData, ATBM_U32 Length)
{
    UINT8 ui8addrData[2] = {BaseReg, OffReg};
    UINT8 retry_times = 10, i;
    ATBM253_ERROR_e ret;
    TV_UNUSE(*pI2CAddr);

    for(i = 0;i < retry_times; i++)
    {
        ret = WriteRegWithDataLen_tv(ui8addrData, 2, (UINT8 *)pData, (UINT16)Length);
        if(ret == ATBM253_NO_ERROR) {
            break;
        } else {
            FE_LOGE("[%s] write regoff=0x%x value=0x%x ret=%d fail\n", __func__, OffReg, *pData, ret);
            ret = ATBM253_ERROR_I2C_FAILED;
        }
    }
    return ret;
}

/********************************************************************
* Function: ATBM253Delay
* Description: Delay. User need to implement this function according to system platform.
*
* Input:     TimeoutUs -- timeout in us to delay
*
* Output: N/A
* Return: N/A
********************************************************************/
void ATBM253Delay(ATBM_U32 TimeoutUs)
{
    (void)TimeoutUs;
}

/********************************************************************
* Function: ATBM253MutexInit
* Description: Initialize one mutex for SDK.
* User need to implement this function according to system platform.
*
* Input:     N/A
*
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253MutexInit(void)
{
#if (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_USER_SPACE)
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_KERNEL_SPACE)
    mutex_init(&ATBM253Mutex);
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_WINDOWS)
    ATBM253Mutex  = CreateSemaphore(ATBM_NULL,1,1,ATBM_NULL);
#else
/*To realize mutex addcording to system platform.*/

#endif
    return ATBM253_NO_ERROR;
}
/********************************************************************
* Function: ATBM253MutexLock
* Description: Lock the mutex of SDK.
* User need to implement this function according to system platform.
*
* Input:    N/A
*
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253MutexLock(void)
{
#if (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_USER_SPACE)
    int ret = 0;
    ret = pthread_mutex_lock(&ATBM253Mutex);
    if(0 != ret)
    {
        return ATBM253_ERROR_UNKNOWN;
    }
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_KERNEL_SPACE)
    mutex_lock(&ATBM253Mutex);
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_WINDOWS)
    WaitForSingleObject(ATBM253Mutex,INFINITE);
#else
/*To realize mutex according to system platform.*/

#endif
    return ATBM253_NO_ERROR;
}

/********************************************************************
* Function: ATBM253MutexUnLock
* Description: Unlock the mutex of SDK.
* User need to implement this function according to system platform.
*
* Input:    N/A
*
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253MutexUnLock(void)
{
#if (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_USER_SPACE)
    int ret = 0;
    ret = pthread_mutex_unlock(&ATBM253Mutex);
    if(0 != ret)
    {
        return ATBM253_ERROR_UNKNOWN;
    }
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_KERNEL_SPACE)
    mutex_unlock(&ATBM253Mutex);
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_WINDOWS)
    ReleaseSemaphore(ATBM253Mutex,1,ATBM_NULL);
#else
/*To realize mutex according to system platform.*/

#endif
    return ATBM253_NO_ERROR;
}


