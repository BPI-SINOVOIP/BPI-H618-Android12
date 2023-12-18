#pragma once

#include <pthread.h>
#include <thread>

#include <utils/Condition.h>
#include <utils/RefBase.h>

#include <gui/Surface.h>
#include <ui/Rect.h>

#include "CommonDefs.h"
#include "SkRenderer.h"
#include "EventQueue.h"
#include "RegionListener.h"

namespace android {

class DrawThread : public virtual RefBase {

public:

    DrawThread(sp<EventQueue> queue, sp<SkRenderer> renderer, const Rect& r);
    ~DrawThread();

    void start();
    void stop();
    void setListener(sp<RegionListener> listener) { mListener = listener; }

private:

    void processInputData();
    bool drawPoint(const InputData& event);
    void doUpdateRegion();
    int getStrokeWitdh(int pressure) {
        if (pressure < PRESSURE_MIN) pressure = PRESSURE_MIN;
        if (pressure > PRESSURE_MAX) pressure = PRESSURE_MAX;
        return STROKE_WIDTH_MIN + (pressure - PRESSURE_MIN) * (STROKE_WIDTH_MAX - STROKE_WIDTH_MIN) / (PRESSURE_MAX - PRESSURE_MIN);
    }

    sp<EventQueue> mEventQueue;
    std::queue<InputData> mPointsQueue;
    sp<SkRenderer> mSkRenderer;
    Rect mScreenRect;

    std::thread mDrawThread;
    bool mStop = false;
    bool mWindowFocus = false;
    bool mFirstUpdatePath = true;

    int mLastX = 0;
    int mLastY = 0;
    int mStrokeWidth;
    SkPath mPath;
    Region mRegion;
    nsecs_t mUpdateRegionTime;

    sp<RegionListener> mListener;

    bool mDebugDrawTime = false;
};

};//namespace android
