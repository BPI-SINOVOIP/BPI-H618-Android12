#ifndef __ATBMTUNERDEFINITION_H__
#define __ATBMTUNERDEFINITION_H__
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef ATBM_BOOL
#define ATBM_BOOL int
#ifndef ATBM_TRUE
#define ATBM_TRUE (1==1)
#define ATBM_FALSE (!ATBM_TRUE)
#endif
#endif


typedef unsigned char       ATBM_U8;
typedef unsigned short		ATBM_U16;
typedef unsigned int		ATBM_U32;
typedef signed char         ATBM_S8;
typedef short               ATBM_S16;
typedef int                 ATBM_S32;


#ifndef ATBM_NULL
#ifdef __cplusplus
#define ATBM_NULL    0
#else
#define ATBM_NULL    ((void *)0)
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif /*__ATBMTUNERDEFINITION_H__*/


