#pragma once

#include <thread>
#include <SkBitmap.h>
#include <gui/Surface.h>
#include <ui/Rect.h>
#include <utils/Condition.h>

#include "RegionListener.h"

namespace android {

class SurfaceUpdater : public RegionListener {

public:

    SurfaceUpdater();
    virtual  ~SurfaceUpdater();

    void start(sp<Surface> surface, SkBitmap* bitmap);
    void stop();
    int onUpdateRegion(const Rect& r, bool needForceRefresh = false) override;
    void onUpdateBuffer() override;

private:

    std::thread mSurfaceThread;
    bool mStop = false;
    bool mNeedRefresh = false;
    Mutex *mMutex;
    Condition *mCondition;

    //bool mFirstUpdate = true;
};

};//namespace android


