#ifndef BUILD_TARGET_RECOVERY
#undef NDEBUG

#include "MultiirService.h"
#include "virtual_input.h"
#include "multiir.h"
#include <binder/IServiceManager.h>
#include <utils/misc.h>
#include <utils/Log.h>


#define DEBUG true

namespace android {

void MultiirService::instantiate() {
    defaultServiceManager()->addService(
        String16("softwinner.multi_ir"), new MultiirService());
}

MultiirService::MultiirService() {
    ALOGD("MultiirService create");
}

MultiirService::~MultiirService() {
    ALOGD("MultiirService destoryed");
}

int MultiirService::enterMouseMode(void) {
    if (DEBUG) ALOGD("MultiirService enterMouseMode");
    int ret = create_virtual_mouse_dev("VirtualMouse");
    return ret;
}

int MultiirService::exitMouseMode(void) {
    if (DEBUG) ALOGD("MultiirService exitMouseMode");
    int ret = destory_virtual_mouse_dev();
    return ret;
}

int MultiirService::getDefaultPointerSpeed(void) {
    return get_default_pointerspeed();
}

int MultiirService::getDefaultStepDistance(void) {
    return get_default_stepdistance();
}

int MultiirService::setPointerSpeed(int mx) {
    set_pointerspeed(mx);
    return 1;
}

int MultiirService::setStepDistance(int px) {
    set_stepdistance(px);
    return 1;
}

int MultiirService::reset(void) {
    reset();
    return 1;
}

int MultiirService::reportMouseKeyEvent(int scan_code, int key_state) {
    return report_mouse_keyevent(scan_code, key_state);
}

int MultiirService::setIRState(int state) {
    set_ir_state(state);
    return 1;
}
}

#endif
