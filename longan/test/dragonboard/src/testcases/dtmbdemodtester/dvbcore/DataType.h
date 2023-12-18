/*
*****************************************************************************

  Copyright (C), 2013-2014, CDT Co., Ltd.

 ******************************************************************************
  File Name     : platformType.h
  Version       : Initial Draft
  Author        : ZhengJH
  Created       : 2013/12/26
  Last Modified :
  Description   : type define
  Function List :
  History       :
  1.Date        : 2013/12/26
    Author      : zhuyuanlv
    Modification: Created file

*****************************************************************************
*/
#ifndef _DATA_TYPE_
#define _DATA_TYPE_
// if define open fujitsu print 
//#define FUJITSU_MSG_PRINT

// change by ylzhu, close DVB_C mode
//#define OPEN_DVB_C_MODE
#define CLOSE_TUNER_UNNECESSARY_FUNCTION
#if 1
typedef unsigned char BYTE;
typedef unsigned char UINT8;
typedef unsigned short int UINT16;
typedef unsigned int UINT32;
typedef char INT8;
typedef short int INT16;
typedef int INT32;
typedef UINT8 BOOL;
#ifndef NULL
	#define NULL    	0
#endif

#ifndef FALSE
	#define FALSE		0
#endif

#ifndef TRUE
	#define TRUE		1
#endif


#endif

typedef signed char			S8;
typedef unsigned char		U8;
typedef signed short		S16;
typedef unsigned short		U16;
typedef signed int			S32;
typedef unsigned int		U32;

typedef int                 INT;
typedef unsigned int        UINT;
typedef long                LONG;
typedef unsigned long       ULONG;
 
typedef volatile U8			VU8;
typedef volatile U16		VU16;
typedef volatile U32		VU32;


typedef void* HANDLE; 

#ifndef NULL
#define NULL 0
#endif

#endif

