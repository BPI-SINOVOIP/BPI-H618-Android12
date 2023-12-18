
#ifndef V4L2_CAMERA_HAL_COMMON_H_
#define V4L2_CAMERA_HAL_COMMON_H_

#include <utils/Log.h>
#include <sys/time.h>
#include <cutils/properties.h>
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include <utils/Trace.h>

#ifndef LOG_TAG
#define LOG_TAG "CameraHALv3_4"
#endif
#define IS_USAGE_VIDEO(usage)  (((usage) & (GRALLOC_USAGE_HW_VIDEO_ENCODER)) \
                         == GRALLOC_USAGE_HW_VIDEO_ENCODER)
#define IS_USAGE_PREVIEW(usage) (((usage) & (GRALLOC_USAGE_HW_TEXTURE)) \
                         == GRALLOC_USAGE_HW_TEXTURE)
#define IS_USAGE_ZSL(usage)  (((usage) & (GRALLOC_USAGE_HW_CAMERA_ZSL)) \
        == (GRALLOC_USAGE_HW_CAMERA_ZSL))


#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_8B(x) (((x) + (7)) & ~(7))
// Debug setting.
#define DBG_V4L2_CAMERA               1
#define DBG_CAMERA                    1
#define DBG_V4L2_WRAPPER              1
#define DBG_V4L2_STREAM               1
#define DBG_STREAM_MANAGER            1
#define DBG_V4L2_GRALLOC              1
#define DBG_CAMERA_CONFIG             1
#define DEBUG_PERFORMANCE             1
// Disable all print information
//#define LOG_NDEBUG 1

/*0.Log.e   1.Log.w   2.Log.i   3.Log.d   4.Log.v*/
#define LOG_LEVEL                     2
#define HAL_LOGE(fmt, args...) if(LOG_LEVEL >= 0) ALOGE("%s:%d: " fmt, __func__, __LINE__, ##args)
#define HAL_LOGW(fmt, args...) if(LOG_LEVEL >= 1) ALOGW("%s:%d: " fmt, __func__, __LINE__, ##args)
#define HAL_LOGI(fmt, args...) if(LOG_LEVEL >= 2) ALOGI("%s:%d: " fmt, __func__, __LINE__, ##args)
#define HAL_LOGD(fmt, args...) if(LOG_LEVEL >= 3) ALOGD("%s:%d %d: " fmt, __func__, __LINE__, LOG_LEVEL,  ##args)
#define HAL_LOGV(fmt, args...) if(LOG_LEVEL >= 4) ALOGV("%s:%d: " fmt, __func__, __LINE__, ##args)
#define HAL_LOGE_IF(cond, fmt, args...) if(LOG_LEVEL == 0) \
    ALOGE_IF(cond, "%s:%d: " fmt, __func__, __LINE__, ##args)
#define HAL_LOGW_IF(cond, fmt, args...) if(LOG_LEVEL > 0) \
    ALOGW_IF(cond, "%s:%d: " fmt, __func__, __LINE__, ##args)
#define HAL_LOGI_IF(cond, fmt, args...) if(LOG_LEVEL > 1) \
    ALOGI_IF(cond, "%s:%d: " fmt, __func__, __LINE__, ##args)

// Log enter/exit of methods.
#define HAL_LOG_ENTER() HAL_LOGV("enter")
#define HAL_LOG_EXIT() HAL_LOGV("exit")

// Fix confliction in case it's defined elsewhere.
#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);  \
  void operator=(const TypeName&);
#endif


// Common setting.

#define DEVICE_FACING_FRONT   1
#define DEVICE_FACING_BACK    0
#define TIMEOUT_COUNT    0
#define MAX_STREAM_NUM 3
#define DROP_BUFFERS_NUM    3
#define MAX_FRAME_NUM 128
// ms delay between stream on and off.
#define DELAY_BETWEEN_STREAM 500
#define DELAY_BETWEEN_ON_OFF 0

#define MAIN_STREAM_PATH "/dev/video0"
#define SUB_0_STREAM_PATH "/dev/video1"
#if (__VIDEO_NUM__ == 2)
#define MAIN_FRONT_STREAM_PATH "/dev/video2"
#define SUB_0_FRONT_STREAM_PATH "/dev/video3"
#else
#define MAIN_FRONT_STREAM_PATH "/dev/video0"
#define SUB_0_FRONT_STREAM_PATH "/dev/video1"
#endif

enum STREAM_SERIAL {
  MAIN_STREAM = 0,
  MAIN_STREAM_BLOB,
  SUB_0_STREAM,
  SUB_0_STREAM_BLOB,
  MAIN_MIRROR_STREAM,
  MAIN_MIRROR_STREAM_BLOB,
  SUB_0_MIRROR_STREAM,
  SUB_0_MIRROR_STREAM_BLOB,
  MAX_STREAM,
  ERROR_STREAM
};


// Platform setting.
#define MAX_BUFFER_CSI_RESERVE 2
#define USE_CSI_VIN_DRIVER
#define USE_ISP
#define MAX_BUFFER_NUM 3
#define PICTURE_BUFFER_NUM 1
#define V4L2_PIX_FMT_DEFAULT V4L2_PIX_FMT_NV21


#ifdef USE_CSI_VIN_DRIVER
#define V4L2_CAPTURE_TYPE V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
#else
#define V4L2_CAPTURE_TYPE V4L2_BUF_TYPE_VIDEO_CAPTURE
#endif

// Tools for save buffers.
#define DBG_BUFFER_SAVE 0
#define DBG_BUFFER_SAVE_ONE_FRAME 1
#define DBG_BUFFER_SAVE_MORE_FRAME 0

#define PATH "/data/camera/"
enum CAMERA_MOUDULE {
    PERFORMANCE = 0,
    TIMESTAMP,
    BUFFER,
    RAW_PIC,
    RAW_VIDEO,
    ZOOM,
    FLASH,
    AF,
    INSERT,
    MAX_MODULE
};
enum DEBUG_MODULE {
    DEBUG_PERF          = 1 << PERFORMANCE,
    DEBUG_TIMESTAMP     = 1 << TIMESTAMP,
    DEBUG_BUFF          = 1 << BUFFER,
    DEBUG_RAW_PIC       = 1 << RAW_PIC,
    DEBUG_RAW_VIDEO     = 1 << RAW_VIDEO,
    DEBUG_ZOOM          = 1 << ZOOM,
    DEBUG_FLASH         = 1 << FLASH,
    DEBUG_AF            = 1 << AF,
    DEBUG_INSERT        = 1 << INSERT,
    DEBUG_NONE          = 1 << MAX_MODULE
};
/*
const char* camera_module_name[MAX_MODULE+1] = {
    "PERFORMANCE",
    "TIMESTAMP",
    "BUFFER",
    "RAW_PIC",
    "RAW_VIDEO",
    "ZOOM",
    "FLASH",
    "AF",
    "NONE"
};
*/

extern void * buffers_addr[MAX_BUFFER_NUM];
extern bool saveBuffers(char *str,void *p, unsigned int length,bool is_oneframe);
extern bool saveSizes(int width, int height);
extern bool isDebugEnable(int32_t module);
extern int getSingleCameraId();
extern int getSupportCameraId(int cameraId);
typedef uint8_t byte;
typedef int32_t int32;
typedef int64_t int64;



#endif  // V4L2_CAMERA_HAL_COMMON_H_
