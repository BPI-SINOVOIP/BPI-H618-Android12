/*
 * Copyright (C) 2017-2018 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file  phOsal_Posix.h
 *  \brief OSAL header files related to operating system functionalities.
 */

#ifndef PHOSAL_POSIX_H_
#define PHOSAL_POSIX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSALSTATUS_OK 0
#define OSALSTATUS_SUCCESS 0
#define OSALSTATUS_INVALID_PARAMS 1
#define OSALSTATUS_FAILED 2
#define OSALSTATUS_SEM_TIMEOUT 3
#define OSALSTATUS_Q_OVERFLOW 4
#define OSALSTATUS_Q_UNDERFLOW 5
#define OSALSTATUS_NOT_SUPPORTED 6
#define OSALSTATUS_NOT_INITIALISED 7
#define OSALSTATUS_INSUFFICIENT_RESOURCES 8
#define OSALSTATUS_ALREADY_INITIALISED 9

typedef int32_t OSALSTATUS; /* Return values */

/** \ingroup grp_osal_log
    Types of logging needed.
    If LOGLEVEL is set for 3(Data Buffers) all types of logs are enabled*/
typedef enum phOsal_eLogLevel {
  PHOSAL_LOGLEVEL_NONE = 0,
  PHOSAL_LOGLEVEL_ERROR = 1,
  PHOSAL_LOGLEVEL_INFO = 2,
  PHOSAL_LOGLEVEL_DEBUG = 3,
  PHOSAL_LOGLEVEL_DATA_BUFFERS = 4
} phOsal_eLogLevel_t;

/**
 * \internal
 *
 * \def PH_OSAL_TRACE_MALLOC
 *
 * Trace what is being allocated and de-allocated.
 *
 * Set to 1 to trace/log malloc and free. 0 to disable tracing */

#if PHFL_LOG_MALLOC_LOGLEVEL == PHFL_LOG_LEVEL_FUNC
#define PH_OSAL_TRACE_MALLOC 1
#else
#define PH_OSAL_TRACE_MALLOC 0
#endif

/** @} */

/** \addtogroup grp_osal_stack
 * @{ */
/**
 * Maximum time a system object can wait to be occupied.
 */
#define PH_OSAL_MAX_WAITTIME (INFINITE)

/**
 * Message Base number at which thread starts storing message queue details */
#define PH_OSAL_MESSAGE_BASE (WM_USER + 0x3FF)

/** \internal
 * Debug mode is enabled to analyze and trace the call stack */
#define PH_OSAL_ENABLE_DEBUG 0

/** @} */
/*
***************************Globals,Structure and Enumeration ******************
*/

/** \addtogroup grp_osal_thread
 * @{ */

/**
 * Thread Function Pointer Declaration.
 *
 * This points to the function to be invoked upon creation of thread.
 * It is not the immediate thread procedure since the API of this function
 * depends on the OS.
 *
 * This function shall be called within the body of the thread procedure
 * defined by the underlying, OS-depending OSAL implementation.
 *
 */
/*typedef void (*pphOsal_ThreadFunction_t)(void*);*/

/** @} */

/** \addtogroup grp_osal_stack
 * @{ */

/**
 * Deferred call declaration.
 * This type of API is called from ClientApplication (main thread) to notify
 * specific callback.
 */
typedef void (*pphOsal_DeferFuncPointer_t)(void*);

/**
 * Deferred message specific info declaration.
 * This type information packed as WPARAM  when windows message
 * is posted to message handler thread.
 */
typedef struct phOsal_DeferedCallInfo {
  pphOsal_DeferFuncPointer_t pDeferedCall; /**< pointer to Deferred callback */
  void* pParam; /**< contains timer message specific details*/
} phOsal_DeferedCallInfo_t;

/** @} */

/** \ingroup grp_osal_exp
 *
 * Supported exception types
 *
 * Enum definition contains supported exception types
 */
typedef enum {
  phOsal_e_NoMemory,           /**< Memory allocation failed */
  phOsal_e_PrecondFailed,      /**< precondition wasn't met */
  phOsal_e_InternalErr,        /**< Unrecoverable error */
  phOsal_e_UnrecovFirmwareErr, /**< Unrecoverable firmware error */
  phOsal_e_TMLerror            /**< Unrecoverable TML error */
} phOsal_ExceptionType_t;      /**< Variable indicating type of Exception */

/** \addtogroup grp_osal_stack
 * @{ */

/**
 *  information to configure OSAL
 */
typedef struct phOsal_Config {
  uint8_t* pLogFile;           /**< Log File Name*/
  uint32_t dwCallbackThreadId; /**< Client ID to which message is posted */
  void* pContext;
} phOsal_Config_t, *pphOsal_Config_t /**< Pointer to #phOsal_Config_t */;

typedef enum phOsal_eSlzType {
  phOsal_eSlzTypeStatic = 0x00,
  phOsal_eSlzTypeDynamic = 0x01
} phOsal_eSlzType_t;

typedef enum phOsal_eSlzVersion {
  phOsal_eSlzVersionMantis = 0x00,
  phOsal_eSlzVersionVienna_1_0 = 0x00,
  phOsal_eSlzVersionVienna_2_0 = 0x00,
  phOsal_eSlzVersionVienna_2_1 = 0x11
} phOsal_eSlzVersion_t;

/**
 * OSAL initialization.
 * This function initializes Timer queue and Critical section variable.
 * \param[in] pOsalConfig               Osal Configuration file.
 * \retval #OSALSTATUS_SUCCESS            Initialization of OSAL module was
 * successful. \retval #OSALSTATUS_INVALID_PARAMS  Client Thread ID passed is
 * invalid. \retval #OSALSTATUS_ALREADY_INITIALISED   Osal Module is already
 * Initialized. \retval #OSALSTATUS_FAILED             Initialization of OSAL
 * module was not successful.
 *
 */
extern OSALSTATUS phOsal_Init(pphOsal_Config_t pOsalConfig);

/**
 * OSAL initialization.
 * This function De-Initializes the Objects and Memory occupied during
 * Initialization. This function also closes the objects if any of it is still
 * open.
 *
 */
extern void phOsal_DeInit(void);

/** @} */

/** \addtogroup grp_osal_rng
 * @{ */

/**
 * Initializes Random Number Generator seed.
 * \note This function executes successfully without OSAL module Initialization.
 */
void phOsal_RandInit(void);

/**
 * Gets Random number. \ref phOsal_RandInit function shall be invoked prior,
 * in order to get non-repeating random numbers.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \retval Random number generated in the range 0 to 32767
 *
 */
unsigned short phOsal_RandGetNumber(void);

/** @} */

/** \addtogroup grp_osal_mm
 * @{ */

/**
 * Allocates some memory.
 *
 * \note This function executes successfully without OSAL module initialization.
 *
 * \param[in] dwSize   Size, in uint32_t, to be allocated
 *
 * \retval NON-NULL value:  The memory is successfully allocated ;
 * the return value is a pointer to the allocated memory location
 * \retval NULL:            The operation is not successful.
 *
 */
#if defined(PH_OSAL_TRACE_MALLOC) && PH_OSAL_TRACE_MALLOC == 0
void* phOsal_GetMemory(uint32_t dwSize);
#else
#define phOsal_GetMemory(dwSize) phOsal_GetMemorySTR(dwSize, #dwSize)
/*
 * Actual function for #phOsal_GetMemory
 * \param dwSize Size of memory to be freed
 * \retval See #phOsal_GetMemory for retval
 */
/* void * phOsal_GetMemoryREAL(uint32_t dwSize); */
/* Run #phOsal_GetMemory and also show what is being allocated */
void* phOsal_GetMemorySTR(uint32_t dwSize, const int8_t* What);
#endif
/**
 * This API allows to free already allocated memory.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pMem  Pointer to the memory block to be deallocated
 */
#if defined(PH_OSAL_TRACE_MALLOC) && PH_OSAL_TRACE_MALLOC == 0
void phOsal_FreeMemory(void* pMem);
#else
#define phOsal_FreeMemory(pMem) phOsal_FreeMemorySTR(pMem, #pMem)
/* Actual function for #phOsal_GetMemory */
/* void phOsal_FreeMemoryREAL(void * pMem); */
/* Run \ref phOsal_FreeMemory and also show what is being freed */
void phOsal_FreeMemorySTR(void* pMem, const int8_t* What);
#endif

/**
 * This API allows to delay the current thread execution.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] dwDelay  Duration in milliseconds for which thread execution to be
 * halted.
 */
void phOsal_Delay(uint32_t dwDelay);

/**
 * Compares the values stored in the source memory with the
 * values stored in the destination memory.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pDest     Pointer to the Destination string.
 * \param[in] pSrc      Pointer to the Source string.
 * \param[in] dwSize    Number of bytes to be compared.
 *
 * \retval Zero value:        The comparison is successful,
                    Both the memory areas contains identical values.
 * \retval Non-Zero Value:    The comparison failed, both the memory
 *                  areas are non-identical.
 *
 */
int32_t phOsal_MemCompare(const void* pDest, const void* pSrc, uint32_t dwSize);

/**
 * Sets the given value in the memory locations.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pMem      Pointer to the memory block to be set to a value
 * \param[in] bVal      Value to be set
 * \param[in] dwSize    Number of bytes to be set.
 *
 */
#if defined(PH_OSAL_TRACE_MALLOC) && PH_OSAL_TRACE_MALLOC == 0
void phOsal_SetMemory(void* pMem, uint8_t bVal, uint32_t dwSize);
#else
#define phOsal_SetMemory(pMem, bVal, dwSize) \
  phOsal_SetMemorySTR(pMem, #pMem, bVal, dwSize, #dwSize)

void phOsal_SetMemoryREAL(void* pMem, uint8_t bVal, uint32_t dwSize);
void phOsal_SetMemorySTR(void* pMem, const int8_t* WhatMem, uint8_t bVal,
                         uint32_t dwSize, const int8_t* WhatSize);
#endif
/**
 * Copies the values stored in the source memory to the
 * values stored in the destination memory.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pDest     Pointer to the Destination Memory
 * \param[in] pSrc      Pointer to the Source Memory
 * \param[in] dwSize    Number of bytes to be copied.
 *
 */
void phOsal_MemCopy(void* pDest, const void* pSrc, uint32_t dwSize);

/** @} */

/** \addtogroup grp_osal_exp
 * @{ */

/**
 * Raises exception on a severe error
 *
 * The program jumps out of the current execution flow, i.e. this function
 * doesn't return. The given exception contains information on what has happened
 * and how severe the error is. \note This function executes successfully
 * without OSAL module Initialization.
 *
 * \param[in] eExceptiontype  exception Type.
 * \param[in] wReason     This is an additional reason value that gives a vendor
 * specific reason code
 */

void phOsal_RaiseException(phOsal_ExceptionType_t eExceptiontype,
                           unsigned short wReason);

/** \addtogroup grp_osal_thread
 * @{ */

/**
 * Thread Function Pointer Declaration.
 *
 * This points to the function to be invoked upon creation of thread.
 * It is not the immediate thread procedure since the API of this function
 * depends on the OS.
 *
 * This function shall be called within the body of the thread procedure
 * defined by the underlying, OS-depending OSAL implementation.
 *
 */
typedef void* (*pphOsal_ThreadFunction_t)(void*);

/** \ingroup grp_osal_thread
 *
 * @{ */

/**
 * Thread Creation.
 *
 * This function creates a thread in the underlying system. To delete the
 * created thread use the phOsal_Thread_Delete function.
 *
 *
 * \param[in,out] hThread    The Thread handle: The caller has to prepare a void
 * pointer that need not to be initialized. The value (content) of the pointer
 * is set by the function.
 *
 * \param[in] pThreadFunction Pointer to a function within the
 *                           implementation that shall be called by the Thread
 *                           procedure. This represents the Thread main
 * function. When this function exits the thread exits. \param[in] pParam A
 * pointer to a user-defined location the thread function receives.
 *
 * \retval #OSALSTATUS_SUCCESS                    The operation was successful.
 * \retval #OSALSTATUS_INSUFFICIENT_RESOURCES     At least one parameter value
 * is invalid. \retval #PH_OSAL_THREAD_CREATION_ERROR     A new Thread could not
 * be created due to system error. \retval #OSALSTATUS_NOT_INITIALISED Osal
 * Module is not Initialized.
 *
 */
OSALSTATUS phOsal_ThreadCreate(void** hThread,
                               pphOsal_ThreadFunction_t pThreadFunction,
                               void* pParam);

/**
 * Get the task identifier of the caller thread.
 *
 * \retval 0 OSAL is not Initialized.
 * \retval NonZero Thread ID of the caller thread
 */
uint32_t phOsal_ThreadGetTaskId(void);

/**
 * Terminates the thread.
 *
 * This function Terminates the thread passed as a handle.
 *
 * \param[in] hThread The handle of the system object.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_INVALID_PARAMS      At least one parameter value is
 * invalid. \retval #PH_OSAL_THREAD_DELETE_ERROR   Thread could not be deleted
 * due to system error. \retval #OSALSTATUS_NOT_INITIALISED        Osal Module
 * is not Initialized.
 *
 */
OSALSTATUS phOsal_ThreadDelete(void* hThread);
#ifdef ENABLE_ADVANCED_FUNCS
/**
 * Suspend the specified thread.
 *
 * \param[in] hThread The handle of the system object.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_INVALID_PARAMS      At least one parameter value is
 * invalid. \retval #PH_OSAL_THREAD_SUSPEND_ERROR  If the thread is not able to
 * be suspended. \retval #OSALSTATUS_NOT_INITIALISED        Osal Module is not
 * Initialized. Note : there is no direct function for suspend thread in linux.
 */
OSALSTATUS phOsal_ThreadSuspend(void* hThread);

/**
 * Resumes the specified thread.
 *
 * \param[in] hThread The handle of the system object.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_INVALID_PARAMS      At least one parameter value is
 * invalid. \retval #PH_OSAL_THREAD_RESUME_ERROR   If the thread is not able to
 * be resume. \retval #OSALSTATUS_NOT_INITIALISED        Osal Module is not
 * Initialized. Note : there is no direct function for wakeup thread in linux.
 */
OSALSTATUS phOsal_ThreadWakeUp(void* hThread);
#endif
/**
 * Set priority for the specified thread.
 *
 * \param[in] hThread       The handle of the system object.
 * \param[in] sdwPriority   Priority of the thread.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_INVALID_PARAMS      At least one parameter value is
 * invalid. \retval #PH_OSAL_THREAD_SETPRIORITY_ERROR If the priority is not
 * able to be set. \retval #OSALSTATUS_NOT_INITIALISED        Osal Module is not
 * Initialized.
 */
OSALSTATUS phOsal_ThreadSetPriority(void* hThread, int32_t sdwPriority);

/**
 * OSAL Message structure
 *
 * Contains message specific details such as message type ,message specific data
 * block details
 */
typedef struct phOsal_Message {
  uint32_t eMsgType; /**< Type of the message to be posted */
  void* pMsgData; /**< Pointer to message specific data block (in case any) */
  unsigned short Size;                  /**< Size of the datablock */
} phOsal_Message_t, *pphOsal_Message_t; /**< pointer to \ref phOsal_Message */

/**
 * Create a queue by allocating specified memory.
 *
 * \param[in,out] msgQHdl    The handle: The caller has to prepare a void
 * pointer that needs not to be initialized. The value (content) of the pointer
 * is set by the function. \param[in] dwQueueLength              Length of the
 * message Queue required.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_INVALID_PARAMS      Any caller provided parameter is
 * invalid. \retval #OSALSTATUS_NOT_INITIALISED        Osal Module is not
 * Initialized.
 *
 */
OSALSTATUS phOsal_MsgQCreate(void** msgQHdl, uint32_t dwQueueLength);

/**
 * This API allows to delete a Message Queue.
 * \param[in] msgQHdl    Handle of the message queue which needs to be deleted.
 *
 * \retval #OSALSTATUS_SUCCESS            The operation was successful.
 * \retval #OSALSTATUS_NOT_INITIALISED   Osal Module is not Initialized.
 * \retval #OSALSTATUS_INVALID_PARAMS  At least one parameter value is invalid.
 */
OSALSTATUS phOsal_MsgQDelete(void* msgQHdl);

/**
 * Function used to post a message to a thread's message queue.
 *
 * \param[in] dwSourceID             The Thread identifier of the sending
 * thread. \param[in] msgQHdl    The handle of the MessageQueue. \param[in] pMsg
 * Pointer to the message structure to be posted to the queue.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_NOT_INITIALISED       Osal Module is not Initialized.
 * \retval #OSALSTATUS_INVALID_PARAMS      Any caller provided parameter is
 * invalid. \retval #OSALSTATUS_INSUFFICIENT_RESOURCES The operation could not
 * complete because of lack of resources.
 */
OSALSTATUS phOsal_MsgQPostMsg(void* msgQHdl, uint32_t dwSourceID,
                              phOsal_Message_t* pMsg);

/**
 * Function used to receive a message from calling thread's message queue.
 *
 * \param[out] pSourceID            Pointer where Thread ID of poster thread is
 * stored. \param[in]  msgQHdl  The handle of the MessageQueue. \param[out] pMsg
 * The message to be filled with the received data from the message queue.
 *
 * \retval #OSALSTATUS_SUCCESS                The operation was successful.
 * \retval #OSALSTATUS_NOT_INITIALISED       Osal Module is not Initialized.
 * \retval #OSALSTATUS_INVALID_PARAMS      Any caller provided parameter is
 * invalid. \retval #OSALSTATUS_INSUFFICIENT_RESOURCES The operation could not
 * complete because of lack of resources.
 */
OSALSTATUS phOsal_MsgQReceiveMsg(void* msgQHdl, uint32_t* pSourceID,
                                 phOsal_Message_t* pMsg);

/**
 * Semaphore Creation.
 * This function creates a semaphore in the underlying system.
 *
 * \param[in,out] hSemaphore The handle: The caller has to prepare a void
 *                           pointer where the handle of semaphore shall be
 * returned.
 *
 * \param[in] bInitialValue   The initial value of the Semaphore.
 * \param[in] bMaxValue       The maximum value of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the
 * function is not Correct. \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     All
 * semaphores are occupied by other threads. \retval
 * #PH_OSAL_SEMAPHORE_CREATION_ERROR  A new semaphore could not be created due
 * to system error. \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is
 * not Initialized.
 *
 */
OSALSTATUS phOsal_SemaphoreCreate(void** hSemaphore, uint8_t bInitialValue,
                                  uint8_t bMaxValue);

/**
 * Semaphore-Produce (never waiting).
 *
 * Increment the value of the semaphore.
 * The purpose is to enable a waiting thread ("consumer") to continue
 * if it has been waiting because of the Semaphore value set to zero.
 *
 * \param[in] hSemaphore The handle of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the
 * function is not Correct. \retval #PH_OSAL_SEMAPHORE_PRODUCE_ERROR   The
 * semaphore cannot be produced due to a system error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
OSALSTATUS phOsal_SemaphorePost(void* hSemaphore);

/**
 * Semaphore Consumption (waiting if value is zero).
 *
 * Decrement the value of the semaphore. When the internal value is
 * non-zero, the function continues. If the value is zero however, the
 * function blocks till the semaphore is released.
 *
 * \param[in] hSemaphore The handle of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the
 * function is not Correct. \retval #PH_OSAL_SEMAPHORE_CONSUME_ERROR   The
 * semaphore can not be consumed due to a system error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
OSALSTATUS phOsal_SemaphoreWait(void* hSemaphore, uint32_t timeout_ms);
/**
 * Semaphore Deletion.
 * This function deletes the Semaphore in the underlying OS.
 *
 * \param[in] hSemaphore                    The handle of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the
 * function is not Correct. \retval #PH_OSAL_SEMAPHORE_DELETE_ERROR    The
 * semaphore can not be deleted due to a system error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
OSALSTATUS phOsal_SemaphoreDelete(void* hSemaphore);

/**
 * Mutex Creation.
 * This function creates a Mutex in the underlying system.
 *
 * \param[in,out] hMutex     The handle: The caller has to prepare a void
 *                           pointer where the handle of mutex shall be
 * returned.
 *
 * \retval #NFCSTATUS_SUCCESS The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the
 * function is not Correct. \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     All
 * Mutexes are occupied by other threads. \retval #PH_OSAL_MUTEX_CREATION_ERROR
 * A new mutex could not be created due to system error. \retval
 * #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
OSALSTATUS phOsal_MutexCreate(void** hMutex);

/**
 * Mutex Locking.
 * This function locks a mutex used for handling critical section of code.
 * However the function blocks till the Mutex is available to be occupied.
 *
 * \param[in] hMutex                    The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS            The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER  Parameter passed to the function is not
 * Correct. \retval #PH_OSAL_MUTEX_LOCK_ERROR  The mutex cannot be locked due to
 * a system error or invalid handle. \retval #NFCSTATUS_NOT_INITIALISED   Osal
 * Module is not Initialized.
 *
 */
OSALSTATUS phOsal_MutexLock(void* hMutex);

/**
 * Mutex Unlocking.
 * This function unlocks a mutex after updating critical section of code.
 *
 * \param[in] hMutex        The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS                The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER      Parameter passed to the function is
 * not Correct. \retval #PH_OSAL_MUTEX_UNLOCK_ERROR    The mutex cannot be
 * locked due to a system error or invalid handle. \retval
 * #NFCSTATUS_NOT_INITIALISED       Osal Module is not Initialized.
 *
 */
OSALSTATUS phOsal_MutexUnlock(void* hMutex);

/**
 * Mutex Deletion.
 * This function deletes a Mutex in the underlying system.
 *
 * \param[in] hMutex        The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS                The operation was successful.
 * \retval #PH_OSAL_MUTEX_DELETE_ERROR    The mutex cannot be deleted due to a
 * system error or invalid handle. \retval #NFCSTATUS_NOT_INITIALISED       Osal
 * Module is not Initialized.
 *
 */
OSALSTATUS phOsal_MutexDelete(void* hMutex);

/**
 * Logging levels
 * This function set the type of logs to be enabled
 *
 * \param[in] eLogLevel  Type of logs to be enabled(info,errors,debug,data)
 *
 */
void phOsal_SetLogLevel(phOsal_eLogLevel_t eLogLevel);

/**
 * Log Error
 * This function prints the error message specified
 * appropriate log level for error needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 *
 */
void phOsal_LogError(const uint8_t* pbMsg);

/**
 * Log Error
 * This function prints the error message specified along with 32bit value in
 * Hex appropriate log level for error needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogErrorU32h(const uint8_t* pbMsg, uint32_t wValue);

/**
 * Log Error
 * This function prints the error message specified along with 32bit value in
 * decimal appropriate log level for error needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 * \param[in] wValue defines value to be printed
 */
void phOsal_LogErrorU32d(const uint8_t* pbMsg, uint32_t wValue);

/**
 * Log Error String
 * This function prints the error message specified along with string
 * appropriate log level for error needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines Error log message to be printed
 * \param[in] pbString defines pointer to string to be printed
 */
void phOsal_LogErrorString(const uint8_t* pbMsg, const uint8_t* pbString);

/**
 * Log Information
 * This function prints the information /warning specified
 * appropriate log level for info needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 *
 */
void phOsal_LogInfo(const uint8_t* pbMsg);

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in Hex appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogInfoU32h(const uint8_t* pbMsg, uint32_t wValue);

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in Hex appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg   defines log message to be printed
 * \param[in] wValue1 defines value to be printed
 * \param[in] wValue2 defines value to be printed
 *
 */
void phOsal_LogInfoU32hh(const uint8_t* pbMsg, uint32_t wValue1,
                         uint32_t wValue2);

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in float appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogInfo32f(const uint8_t* pbMsg, float wValue);

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in decimal appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg  defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogInfoU32d(const uint8_t* pbMsg, uint32_t wValue);

/**
 * Log Information
 * This function prints the information /warning specified along with 32bit
 * value in decimal appropriate log level for info needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg   defines log message to be printed
 * \param[in] wValue1 defines value to be printed
 * \param[in] wValue2 defines value to be printed
 *
 */
void phOsal_LogInfoU32dd(const uint8_t* pbMsg, uint32_t wValue1,
                         uint32_t wValue2);

/**
 * Log Information and String
 * This function prints the information specified along with string
 * appropriate log level for info needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] pbString defines pointer to string to be printed
 */
void phOsal_LogInfoString(const uint8_t* pbMsg, const uint8_t* pbString);

/**
 * Log Debug Information
 * This function prints the Debug log message specified
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 *
 */
void phOsal_LogDebug(const uint8_t* pbMsg);

/**
 * Log Debug Information
 * This function prints the Debug log message specified along with 32bit value
 * in hex appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogDebugU32h(const uint8_t* pbMsg, uint32_t wValue);

/**
 * Log Debug Information
 * This function prints the Debug log message specified along with 32bit value
 * in decimal appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogDebugU32d(const uint8_t* pbMsg, uint32_t wValue);

/**
 * Log Debug Information
 * This function prints the Debug log message specified along with pointer value
 * in hex appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] pValue defines value to be printed
 *
 */
void phOsal_LogDebugPtrh(const uint8_t* pbMsg, void* pValue);

/**
 * Log Debug Information specified along with the string
 * This function prints the Debug log message specified along with 32bit value
 * in decimal appropriate log level for debug needs to be enabled from
 * phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogDebugString(const uint8_t* pbMsg, const uint8_t* pbString);

/**
 * Log function entry point
 * This function prints the function name specified prefixed by Module name
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbModuleName defines log message to be printed
 * \param[in] pbFuncName defines value to be printed
 *
 */
void phOsal_LogFunctionEntry(const uint8_t* pbModuleName,
                             const uint8_t* pbFuncName);

/**
 * Log function exit point
 * This function prints the function name specified prefixed by Module name
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbMsg defines log message to be printed
 * \param[in] wValue defines value to be printed
 *
 */
void phOsal_LogFunctionExit(const uint8_t* pbModuleName,
                            const uint8_t* pbFuncName);

/**
 * Log Buffer data
 * This function prints the data in the buffers provided in Hex
 * appropriate log level for debug needs to be enabled from phOsal_SetLogLevel
 *
 * \param[in] pbBuffer              Buffer data  to be logged
 * \param[in] dwSizeOfBuffer        Size of buffer data.
 * \param[in] pbMsg                 defines message to be printed before
 * printing the buffer data.
 *
 */
void phOsal_LogBuffer(const uint8_t* pbBuffer, uint32_t dwSizeOfBuffer,
                      const uint8_t* pbMsg);

#ifdef __cplusplus
}
#endif /*  C++ Compilation guard */

#endif
