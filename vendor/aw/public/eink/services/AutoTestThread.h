#pragma once

#include <thread>
#include <ui/Rect.h>
#include <utils/Condition.h>

#include "EventQueue.h"

namespace android {

class AutoTestThread : public virtual RefBase {

public:

    AutoTestThread(sp<EventQueue> queue, Rect& layout, int interval, int pressureMin, int pressureMax, int step, int stepInterval);
    AutoTestThread(sp<EventQueue> queue, Rect& layout, int mode, int xstep, int ystep, int interval);
    ~AutoTestThread();

    void start();
    void stop();

private:
    void sendRandomPoint();
    void sendFixedLine(int x1, int x2, int y1, int y2);
    void sendFixedPoint(int x, int y, int type);

    std::thread mTestThread;
    bool mStop= false;
    Mutex *mMutex;

    sp<EventQueue> mEventQueue;
    Rect mLayout;
    int mInterval = 120; /* second */
    int mPressureMin = PRESSURE_MIN;
    int mPressureMax = PRESSURE_MAX;
    int mStep = 0;
    int mStepInterval = 0;  /* millisecond */
    int mStepCount = 1;

    int mFixLineMode = 0;
    int mFixLineXStep = 50;
    int mFixLineYStep = 50;
    int mFixLineInterval = 10;/* ms */
};

};//namespace android
