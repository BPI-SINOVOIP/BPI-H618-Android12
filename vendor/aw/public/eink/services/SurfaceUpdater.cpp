#define LOG_NDEBUG 0
#define LOG_TAG "SurfaceUpdater"

#include <android-base/logging.h>
#include "CommonDefs.h"
#include "DisplayConfigHal.h"
#include "SurfaceUpdater.h"

namespace android {

SurfaceUpdater::SurfaceUpdater() {
    mMutex = new Mutex();
    mCondition = new Condition();
}

SurfaceUpdater::~SurfaceUpdater() {
    LOG(DEBUG) << "~SurfaceUpdater";

    if (!mStop) {
        stop();
    } else {
        //for thread actively exiting
        if (mSurfaceThread.joinable()) {
            mSurfaceThread.join();
        }
    }

    delete mCondition;
    delete mMutex;
}

void SurfaceUpdater::start(sp<Surface> surface, SkBitmap* bitmap) {
    mStop = false;
    mNeedRefresh  = false;
    mSurfaceThread = std::thread([this, surface, bitmap]() {
        LOG(DEBUG) << "Surface thread start";
        ANativeWindow_Buffer info;
        int ret;
        int size = bitmap->computeByteSize();
        LOG(DEBUG) << "bitmap->computeByteSize: " << size;
        while (!mStop) {
            mMutex->lock(); {
                // TODO lock dirty rect
                ret = surface->lock(&info, nullptr);
                //LOG(DEBUG) << "update surface " << ret;
                if (ret != 0) {
                    mStop = true;
                    mMutex->unlock();
                    continue;
                }
                memcpy(info.bits, bitmap->getPixels(), size);
                ret = surface->unlockAndPost();
                if (!mNeedRefresh) {
                    mCondition->wait(*mMutex);
                }
                mNeedRefresh = false;
                usleep(16000);
            } mMutex->unlock();
        }
        LOG(DEBUG) << "Surface thread stop";
    });
}

void SurfaceUpdater::stop() {
    LOG(DEBUG) << "SurfaceUpdater stop";
    mStop = true;
    mMutex->lock(); {
        mCondition->signal();
    } mMutex->unlock();
    if (mSurfaceThread.joinable()) {
        mSurfaceThread.join();
    }
}

int SurfaceUpdater::onUpdateRegion(const Rect& r, bool needForceRefresh) {
    if (r.left >= r.right || r.top >= r.bottom) {
        LOG(ERROR) << "onUpdateRegion invalid param";
        return 0;
    }

    /*
     * HWC does not refresh the non-handwriting layer in handwriting mode,
     * so that it can't enter handwriting mode too early, for example, when
     * DrawThread starting, which lead to Buttons not show.
     */
    //if (mFirstUpdate) {
    //    DisplayConfigHal::getInstance()->setRefreshMode(IEinkMode::DU, true);
    //    mFirstUpdate = false;
    //}
    //LOG(DEBUG) << "onUpdateRegion: [" << r.left << ", " << r.top << ", " << r.right << ", " << r.bottom << "]";

    /*
     * In handwriting and DU mode, the effect of drawing background is poor,
     * so that force refresh, that is, GC16 mode.
     */
    if (needForceRefresh) {
        DisplayConfigHal::getInstance()->forceGlobalRefresh(true);
    }

    int ret = RET_FAIL;
    ret = DisplayConfigHal::getInstance()->updateDamage(r);
    if (ret == RET_OK) {
        mNeedRefresh = true;
    }
    return ret;
}

void SurfaceUpdater::onUpdateBuffer() {
    mCondition->signal();
}

};
