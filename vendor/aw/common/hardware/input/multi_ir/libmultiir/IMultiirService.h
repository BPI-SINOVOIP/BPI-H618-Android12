#ifndef ANDROID_IMULTIIRSERVICE_H
#define ANDROID_IMULTIIRSERVICE_H

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/Log.h>

namespace android {
class IMultiirService : public IInterface {

public:
    DECLARE_META_INTERFACE(MultiirService);
    virtual int enterMouseMode(void) = 0;
    virtual int exitMouseMode(void) = 0;
    virtual int getDefaultPointerSpeed(void) = 0;
    virtual int getDefaultStepDistance(void) = 0;
    virtual int setPointerSpeed(int ms) = 0;
    virtual int setStepDistance(int px) = 0;
    virtual int reset(void) = 0;
    virtual int reportMouseKeyEvent(int scan_code, int key_state) = 0;
    virtual int setIRState(int state) = 0;
};

class BnMultiirService : public BnInterface<IMultiirService> {

public:
    virtual status_t  onTransact(uint32_t code, const Parcel& data,
                                 Parcel* reply, uint32_t flags = 0);
};
};

#endif
