#ifndef ANDROID_MULTIIRSERVICE_H
#define ANDROID_MULTIIRSERVICE_H
#ifndef BUILD_TARGET_RECOVERY

#include <utils/Log.h>
#include <utils/Errors.h>
#include "IMultiirService.h"

namespace android {

class MultiirService : public BnMultiirService {
public:
    static void instantiate();
    virtual int enterMouseMode(void);
    virtual int exitMouseMode(void);
    virtual int getDefaultPointerSpeed(void);
    virtual int getDefaultStepDistance(void);
    virtual int setPointerSpeed(int ms);
    virtual int setStepDistance(int px);
    virtual int reset(void);
    virtual int reportMouseKeyEvent(int scan_code, int key_state);
    virtual int setIRState(int state);

private:
    MultiirService();
    virtual ~MultiirService();
};

};
#endif

#endif
