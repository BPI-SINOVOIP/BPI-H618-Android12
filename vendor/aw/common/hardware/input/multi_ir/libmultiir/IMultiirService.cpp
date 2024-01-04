#undef NDEBUG
#define LOG_TAG "MultiirService"

#include <utils/Log.h>
#include <memory.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include "IMultiirService.h"

#define DEBUG true

namespace android {

enum {
    ENTERMOUSEMODE = IBinder::FIRST_CALL_TRANSACTION,       // 1
    EXITMOUSEMODE = ENTERMOUSEMODE + 1,                     // 2
    GETDEFAULTPOINTERSPEED = EXITMOUSEMODE + 1,             // 3
    GETDEFAULTSTEPDISTANCE = GETDEFAULTPOINTERSPEED + 1,    // 4
    SETPOINTERSPEED = GETDEFAULTSTEPDISTANCE + 1,           // 5
    SETSTEPDISTANCE = SETPOINTERSPEED + 1,                  // 6
    RESET = SETSTEPDISTANCE + 1,                            // 7
    REPORTMOUSEKEYEVENT = RESET + 1,                        // 8
    SETIRSTATE = REPORTMOUSEKEYEVENT + 1,                   // 9
};

class BpMultiirService : public BpInterface<IMultiirService> {

public:
    BpMultiirService(const sp<IBinder>& impl)
        : BpInterface<IMultiirService>(impl){
    }

    int enterMouseMode(void) {
        if (DEBUG) {
            ALOGD("IMultiirService enter virtual mouse mode");
        }
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        remote()->transact(ENTERMOUSEMODE, data, &reply);
        return reply.readInt32();
    }

    int exitMouseMode(void) {
        if (DEBUG) {
            ALOGD("IMultiirService exit virtual mouse mode");
        }
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        remote()->transact(EXITMOUSEMODE, data, &reply);
        return reply.readInt32();
    }

    int getDefaultPointerSpeed(void) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        remote()->transact(GETDEFAULTPOINTERSPEED, data, &reply);
        return reply.readInt32();
    }

    int getDefaultStepDistance(void) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        remote()->transact(GETDEFAULTSTEPDISTANCE, data, &reply);
        return reply.readInt32();
    }

    int reset(void) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        remote()->transact(RESET, data, &reply);
        return reply.readInt32();
    }

    int setPointerSpeed(int ms) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        data.writeInt32(ms);
        remote()->transact(SETPOINTERSPEED, data, &reply);
        return reply.readInt32();
    }

    int setStepDistance(int px) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        data.writeInt32(px);
        remote()->transact(SETSTEPDISTANCE, data, &reply);
        return reply.readInt32();
    }
    int reportMouseKeyEvent(int scan_code, int key_state) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        data.writeInt32(scan_code);
        data.writeInt32(key_state);
        remote()->transact(REPORTMOUSEKEYEVENT, data, &reply);
        return reply.readInt32();
    }
    int setIRState(int state) {
        Parcel data, reply;
        data.writeInterfaceToken(IMultiirService::getInterfaceDescriptor());
        data.writeInt32(state);
        remote()->transact(SETPOINTERSPEED, data, &reply);
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(MultiirService, "com.softwinner.IMultiirService");

status_t BnMultiirService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    switch (code) {
        case ENTERMOUSEMODE :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            reply->writeInt32(enterMouseMode());
            return NO_ERROR;
        }
        case EXITMOUSEMODE :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            reply->writeInt32(exitMouseMode());
            return NO_ERROR;
        }
        case GETDEFAULTPOINTERSPEED :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            reply->writeInt32(getDefaultPointerSpeed());
            return NO_ERROR;
        }
        case GETDEFAULTSTEPDISTANCE :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            reply->writeInt32(getDefaultStepDistance());
            return NO_ERROR;
        }
        case SETPOINTERSPEED :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            int speed = data.readInt32();
            reply->writeInt32(setPointerSpeed(speed));
            return NO_ERROR;
        }
        case SETSTEPDISTANCE :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            int distance = data.readInt32();
            reply->writeInt32(setStepDistance(distance));
            return NO_ERROR;
        }
        case RESET :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            reply->writeInt32(reset());
            return NO_ERROR;
        }
        case REPORTMOUSEKEYEVENT :
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            int scan_code = data.readInt32();
            int key_state = data.readInt32();
            reply->writeInt32(reportMouseKeyEvent(scan_code, key_state));
            return NO_ERROR;
        }
        case SETIRSTATE:
        {
            CHECK_INTERFACE(IMultiitService, data, reply);
            int state = data.readInt32();
            reply->writeInt32(setIRState(state));
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

};

