
#ifndef __APPERCEIVEPEOPLE_API_H___
#define __APPERCEIVEPEOPLE_API_H___

#include <utils/StrongPointer.h>

namespace android {

struct APPERCEIVEPEOPLE_INFO
{
    int scree_oriention;//横竖屏，横屏：1，竖屏：2
    int buffer_oriention; //buffer_oriention正常0，反了是1.
};

typedef int (*apperceive_notify_cb)(int cmd, void * data, void *user);

enum APPERCEIVEPEOPLE_NOTITY_CMD{
    APPERCEIVEPEOPLE_NOTITY_CMD_REQUEST_FRAME,
    APPERCEIVEPEOPLE_NOTITY_CMD_RESULT,
    APPERCEIVEPEOPLE_NOTITY_CMD_POSITION,
    APPERCEIVEPEOPLE_NOTITY_CMD_REQUEST_ORIENTION,
};

class CApperceivePeople;

enum APPERCEIVEPEOPLE_OPS_CMD
{
    APPERCEIVEPEOPLE_OPS_CMD_START,
    APPERCEIVEPEOPLE_OPS_CMD_STOP,
    APPERCEIVEPEOPLE_OPS_CMD_REGISTE_USER,
};

struct ApperceivePeopleDev
{
    void * user;
    sp<CApperceivePeople> priv;
    void (*setCallback)(ApperceivePeopleDev * dev, apperceive_notify_cb cb);
    int (*ioctrl)(ApperceivePeopleDev * dev, int cmd, int para0, int para1,void* arg ,int mode_idx);
};

extern int CreateApperceivePeopleDev(ApperceivePeopleDev ** dev);
extern void DestroyApperceivePeopleDev(ApperceivePeopleDev * dev);

}

#endif