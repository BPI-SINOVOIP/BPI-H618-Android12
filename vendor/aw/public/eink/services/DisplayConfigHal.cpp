#define LOG_NDEBUG 0
#define LOG_TAG "DisplayConfigHal"

#include <android-base/logging.h>
#include <cutils/native_handle.h>

#include "CommonDefs.h"
#include "DisplayConfigHal.h"

namespace android {

DisplayConfigHal* DisplayConfigHal::getInstance() {
    static DisplayConfigHal* instance = new DisplayConfigHal();
    return instance;
}

DisplayConfigHal::DisplayConfigHal() {
    mDisplayConfig = IDisplayConfig::getService();
}

void DisplayConfigHal::setRefreshMode(int mode, bool handwrite) {
    LOG(DEBUG) << "DisplayConfigHal::setRefreshMode mode:" << mode << " handwrite:"<<handwrite;
    int hwcmode = (int)kModeMap[mode];
    if (handwrite)
        hwcmode |= (int)hwc2_eink_refresh_mode_t::HWC_EINK_HANDWRITTEN;
    if (mDisplayConfig != nullptr && hwcmode != 0) {
        mDisplayConfig->setEinkMode(hwcmode);
    } else {
        LOG(WARNING) << "Error DisplayConfig or mode: " <<  hwcmode;
    }
}

void DisplayConfigHal::setBuffer(int fd) {
    if (mDisplayConfig != nullptr) {
        native_handle_t* handle = native_handle_create(/*numFds=*/1, /*numInts=*/0);
        handle->data[0] = fd;
        hardware::hidl_handle bufferInfo;
        bufferInfo.setTo(handle, /*shouldOwn=*/false);
        mDisplayConfig->setEinkBufferFd(bufferInfo);
    }
}

int DisplayConfigHal::updateDamage(const Rect& r) {
    int ret = RET_FAIL;
    if (mDisplayConfig != nullptr) {
        ret = mDisplayConfig->updateEinkRegion(r.left, r.top, r.right, r.bottom);
    }
    return ret;
}

int DisplayConfigHal::forceGlobalRefresh(bool rightNow) {
    if (mDisplayConfig != nullptr) {
        return mDisplayConfig->forceEinkRefresh(rightNow);
    } else {
        LOG(WARNING) << "Error DisplayConfig";
        return -1;
    }
}

int DisplayConfigHal::setHandwrittenArea(const Rect& r) {
    if (mDisplayConfig != nullptr) {
        return mDisplayConfig->setEinkUpdateArea(r.left, r.top, r.right, r.bottom);
    } else {
        LOG(ERROR) << "Error DisplayConfig";
        return -1;
    }
}
};
