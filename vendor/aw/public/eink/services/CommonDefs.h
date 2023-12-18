#pragma once

//#define LOG_NDEBUG 0

#include <stdio.h>
#include <utils/Log.h>
#include <cutils/list.h>

#define DEBUG_FLAG
#ifdef DEBUG_FLAG
#define PR_INFO ALOGD
#define PR_WARN ALOGW
#define PR_ERR  ALOGE
#else
#define PR_INFO
#define PR_WARN     ALOGW
#define PR_ERR      ALOGE
#endif

#define unusedpara(x) x=x

#define MIN(a,b) ((a)<(b))?(a):(b)
#define MAX(a,b) ((a)<(b))?(b):(a)

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#define RECT_WIDTH  50
#define RECT_HEIGHT 50
#define RECT_HOR_STEP   20
#define RECT_VER_STEP   20

#define BLACK           0x00
#define WHITE           0xff

#define FRESH_TIME      5

#define ACTION_DOWN 0x0
#define ACTION_UP   0x1
#define ACTION_MOVE 0x2
#define LAST_PATH_FILE  "/data/eink/pen.png"

#define STROKE_WIDTH_MIN 2
#define STROKE_WIDTH_MAX 10
#define PRESSURE_MIN 0
#define PRESSURE_MAX 4095

enum {
    EVENT_TYPE_PEN,
    EVENT_TYPE_RUBBER,
    EVENT_TYPE_TOUCH,
    EVENT_TYPE_CLEAR,
    EVENT_TYPE_REFRESH,
    EVENT_TYPE_STOP,
    EVENT_TYPE_BACKGROUND,
    EVENT_TYPE_SAVE,
    EVENT_TYPE_FOCUS,
};

enum {
    RET_FAIL = -1,
    RET_OK = 0,
};
// data struct:
typedef struct {
    int type;
    int x;
    int y;
    int pressure;
    int eventType;
    void* data;
} InputData;

typedef struct {
    // android: cutils/list.h
    struct listnode node;
    InputData       data;
} InputDataListNode;

