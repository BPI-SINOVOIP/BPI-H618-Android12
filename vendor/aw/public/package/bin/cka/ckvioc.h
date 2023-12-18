#ifndef CKV_IOCTL
#define CKV_IOCTL
/*
  NOTE: DEC C on OpenVMS AXP does not like an empty header file,
  so we include the following system header files unconditionally.
*/
#include <errno.h>
#include <stdio.h>

#ifdef DEC_TCPIP
/*
  The rest is used only if we have selected DEC TCP/IP support.
*/
#include <iodef.h>
#ifdef TCPWARE
#include "tcpware_include:ucx$inetdef.h"
#else
#include <ucx$inetdef.h>
#endif /* TCPWARE */

#ifndef _IO
#define IOCPARM_MASK   0x7f		/* Parameters are < 128 bytes */
#define IOC_VOID       (int)0x20000000	/* No parameters */
#define IOC_OUT        (int)0x40000000	/* Copy out parameters */
#define IOC_IN         (int)0x80000000	/* Copy in parameters */
#define IOC_INOUT      (int)(IOC_IN|IOC_OUT)
#define _IO(x,y)       (int)(IOC_VOID|('x'<<8)|y)
#define _IOR(x,y,t)    (int)(IOC_OUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define _IOW(x,y,t)    (int)(IOC_IN|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define _IOWR(x,y,t)   (int)(IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)`
#endif /* _IO */
#endif /* DEC_TCPIP */
#endif /* CKV_IOCTL */
