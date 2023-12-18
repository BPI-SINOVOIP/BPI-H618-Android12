/********************************************************************************************************
*File name: ATBM253Porting.h
*Description: Headerfile of ATBM253Porting.c. The functions in this file are called by ATBM253 SDK, and they should be implemented
*            according to specific system platform.
*
*********************************************************************************************************/

#ifndef __ATBM253PORTING_H__
#define __ATBM253PORTING_H__
#include "ATBM253Api.h"
#include "fe_log.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*The count of ATBM253 tuner in the system.*/
#define ATBM253_TUNER_COUNT (2)

#define ATBM253_TUNER_DTMB_SUPPORT  1   /*SDK default support DTMB */
#define ATBM253_TUNER_DVBT_SUPPORT  1    /*SDK default support DVBT */
#define ATBM253_TUNER_DVBC_SUPPORT  1    /*SDK default support DVBC */
#define ATBM253_TUNER_ATSC_SUPPORT  1    /*SDK default support ATSC */
#define ATBM253_TUNER_ISDBT_SUPPORT 1    /*SDK default support ISDBT*/
#define ATBM253_TUNER_ATV_SUPPORT   1    /*SDK default support ATV  */
/*System OS type.*/
#define ATBM253_OS_TYPE_LINUX_USER_SPACE (0) /* ATBM253 working in Linux user space. */
#define ATBM253_OS_TYPE_LINUX_KERNEL_SPACE (1) /* ATBM253 working in Linux kernel space. */
#define ATBM253_OS_TYPE_WINDOWS (2) /* ATBM253 working in Windows. */
#define ATBM253_OS_TYPE_CUSTOM (3) /* ATBM253 working in customer OS type. */

/* Select OS type according to system platform. */
#define ATBM253_OS_TYPE_SEL (ATBM253_OS_TYPE_LINUX_USER_SPACE)


#ifdef ATBM253_CHIP_DEBUG_OPEN
#undef ATBM253_OS_TYPE_SEL
#define ATBM253_OS_TYPE_SEL (ATBM253_OS_TYPE_WINDOWS)
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
ATBM253_ERROR_e ATBM253I2CRead(ATBM253I2CAddr_t *pI2CAddr,ATBM_U8 BaseReg,ATBM_U8 OffReg,ATBM_U8 *pData, ATBM_U32 Length);

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
ATBM253_ERROR_e ATBM253I2CWrite(ATBM253I2CAddr_t *pI2CAddr,ATBM_U8 BaseReg,ATBM_U8 OffReg,ATBM_U8 *pData, ATBM_U32 Length);

/********************************************************************
* Function: ATBM253Delay
* Description: Delay. User need to implement this function according to system platform.
*
* Input:     TimeoutUs -- timeout in us to delay
*
* Output: N/A
* Retrun: N/A
********************************************************************/
void ATBM253Delay(ATBM_U32 TimeoutUs);

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
ATBM253_ERROR_e ATBM253MutexInit(void);

/********************************************************************
* Function: ATBM253MutexLock
* Description: Lock the mutex of SDK.
* User need to implement this function according to system platform.
*
* Input:     N/A
*
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253MutexLock(void);

/********************************************************************
* Function: ATBM253MutexUnLock
* Description: Unlock the mutex of SDK.
* User need to implement this function according to system platform.
*
* Input:     N/A
*
* Output: N/A
* Retrun: ATBM253_ERROR_NO_ERROR -- no error; others refer to ATBM253_ERROR_e
********************************************************************/
ATBM253_ERROR_e ATBM253MutexUnLock(void);


/********************************************************************
*
* Customer configuration macro definitions
*
********************************************************************/



/********************************************************************
* ATBM253 debug output control.
* If define 'ATBM253_DEBUG_PRINT' to '0', debug output will be closed;
*   otherwise,debug output will be opened.
* The default implement of 'ATBM253Print' is 'printf', and it can be changed according to
*   system platform.
********************************************************************/
#define ATBM253_DEBUG_PRINT (1)

#if ATBM253_DEBUG_PRINT
#if((ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_USER_SPACE)||(ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_WINDOWS))
#include <stdio.h>
#define ATBM253Print(x) printf x
#elif (ATBM253_OS_TYPE_SEL == ATBM253_OS_TYPE_LINUX_KERNEL_SPACE)
#include <linux/kernel.h>
#define ATBM253Print(x) printk x
#else
#define ATBM253Print(x)   printf x   /*SOC print function*/
#endif

#else
#define ATBM253Print(x)  /* x */
#endif


#ifdef __cplusplus
}
#endif

#endif /*__ATBM253PORTING_H__*/

