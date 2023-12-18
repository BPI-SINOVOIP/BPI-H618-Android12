#ifndef __LIBVE_DECORDER2_H__
#define __LIBVE_DECORDER2_H__

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <pthread.h>
#include <vencoder.h>  //* video encode library in "LIBRARY/CODEC/VIDEO/ENCODER"
#include <vdecoder.h>
#include <memoryAdapter.h>

#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <cutils/properties.h>
#define DBG_ENABLE 0

#ifdef __cplusplus
extern "C" {
#endif

#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_8B(x) (((x) + (7)) & ~(7))

extern void AddVDPlugin(void);
extern void AddVDPluginSingle(char *lib);

int Libve_dec2(VideoDecoder** mVideoDecoder,
               const void *in,
               void *outY,
               void *outC,
               VideoStreamInfo* pVideoInfo,
               VideoStreamDataInfo* dataInfo,
               VConfig* pVconfig);
int  Libve_init2(VideoDecoder** mVideoDecoder,
                 VideoStreamInfo* pVideoInfo,
                 VConfig* pVconfig);
int  Libve_exit2(VideoDecoder** mVideoDecoder);

#ifdef __cplusplus
}
#endif


#endif  /* __LIBVE_DECORDER2_H__ */

