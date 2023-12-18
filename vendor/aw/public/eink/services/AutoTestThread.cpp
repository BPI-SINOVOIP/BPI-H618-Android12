#define LOG_NDEBUG 0
#define LOG_TAG "AutoTestThread"

#include <android-base/logging.h>
#include "AutoTestThread.h"

#define INTERVAL_PER_PATH 100
#define random(x) (rand()%(x))

#define FIX_LINE_OFFSET 100

namespace android {

AutoTestThread::AutoTestThread(sp<EventQueue> queue,
    Rect& layout,  int interval, int pressureMin, int pressureMax,
    int step, int stepInterval):
    mEventQueue(queue), mPressureMin(pressureMin),
    mStep(step), mStepInterval(stepInterval){
    mMutex = new Mutex();

    mLayout.left = layout.left;
    mLayout.top = layout.top;
    mLayout.right = layout.right;
    mLayout.bottom = layout.bottom;

    if (pressureMax != 0) {
        mPressureMax = pressureMax;
    }

    if (interval != 0) {
        mInterval = interval;
    }

    if (mStep == 0) {
        mStep = (mLayout.right-mLayout.left)/40;
    }

    if (mStepInterval  == 0) {
        mStepInterval = INTERVAL_PER_PATH;
    }
    mStepCount = 1000/mStepInterval;
    LOG(DEBUG) << "Test layout: [" << mLayout.left << ", " << mLayout.top << ", " << mLayout.right << ", " << mLayout.bottom << "]";
    LOG(DEBUG) << "Test param: [" << mInterval << ", " << mPressureMin << ", " << mPressureMax << ", " << mStep << ", " << mStepInterval << "]";
}

AutoTestThread::AutoTestThread(sp<EventQueue> queue,
    Rect& layout, int mode, int xstep, int ystep, int interval):
    mEventQueue(queue){
    mMutex = new Mutex();

    mLayout.left = layout.left;
    mLayout.top = layout.top;
    mLayout.right = layout.right;
    mLayout.bottom = layout.bottom;

    mFixLineMode = mode;
    mFixLineXStep = xstep;
    mFixLineYStep = ystep;
    if (interval != 0) {
        mFixLineInterval = interval;
    }
    LOG(DEBUG) << "Test layout: [" << mLayout.left << ", " << mLayout.top << ", " << mLayout.right << ", " << mLayout.bottom << "]";
}

AutoTestThread::~AutoTestThread() {
    if (!mStop) {
        stop();
    }
    delete mMutex;
}

void AutoTestThread::sendFixedLine(int x1, int x2, int y1, int y2) {
    InputData point;
    point.x = x1;
    point.y = y1;
    point.pressure = (mPressureMin + mPressureMax)/2;
    point.eventType = EVENT_TYPE_PEN;
    point.type = ACTION_DOWN;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input1: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";

    point.x = x2;
    point.y = y2;
    point.type = ACTION_MOVE;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input2: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";

    point.type = ACTION_UP;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input3: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";
}

void AutoTestThread::sendFixedPoint(int x, int y, int type) {
    InputData point;
    point.x = x;
    point.y = y;
    point.pressure = (mPressureMin + mPressureMax)/2;
    point.eventType = EVENT_TYPE_PEN;
    point.type = type;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input1: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";
}

void AutoTestThread::sendRandomPoint(){
    int viewWidth = mLayout.right-mLayout.left;
    int viewHeight = mLayout.bottom-mLayout.top;
    int x0= mLayout.left+random(viewWidth);
    int y0= mLayout.top+random(viewHeight);
    InputData point;
    point.x = x0;
    point.y = y0;
    point.pressure = mPressureMin + random(mPressureMax - mPressureMin);
    point.eventType = EVENT_TYPE_PEN;
    point.type = ACTION_DOWN;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input1: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";

    int x1= x0+mStep>mLayout.right?mLayout.right:x0+mStep;
    int y1= y0+mStep>mLayout.bottom?mLayout.bottom:y0+mStep;
    point.x = x1;
    point.y = y1;
    point.pressure = mPressureMin + random(mPressureMax - mPressureMin);
    point.eventType = EVENT_TYPE_PEN;
    point.type = ACTION_MOVE;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input2: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";

    point.x = x1;
    point.y = y1;
    point.type = ACTION_UP;
    mEventQueue->enqueue(point);
    //LOG(DEBUG) << "send input3: [" << point.x << ", " << point.y << ", " << point.pressure << ", " << point.eventType << ", " << point.type <<"]";
}

void AutoTestThread::start() {
    mStop = false;
    mTestThread = std::thread([this]() {
        LOG(DEBUG) << "Test thread start";
        if (mFixLineMode == 0) {
            int count = 0;
            srand((int)time(0));
            while (!mStop) {
                mMutex->lock(); {
                    if(count < mStepCount * mInterval){
                        count++;
                        sendRandomPoint();
                    }else{
                        count = 0;
                        //to do save
                        LOG(DEBUG) << "clear test path";
                        mEventQueue->enqueue(EVENT_TYPE_CLEAR);
                        sleep(1);
                    }
                    usleep(mStepInterval*1000);
                } mMutex->unlock();
            }
        } else {
            int x0 = mLayout.left + FIX_LINE_OFFSET;
            int y0 = mLayout.top + FIX_LINE_OFFSET;
            while (!mStop) {
                mMutex->lock(); {
                    if (y0+FIX_LINE_OFFSET < mLayout.bottom) {
                        int x=x0;
                        if (mFixLineXStep == 0) {
                            x=y0;
                        }

                        int y=y0;
                        sendFixedPoint(x, y, ACTION_DOWN);
                        for(; (x+mFixLineXStep<mLayout.right) && (y+mFixLineYStep<mLayout.bottom);
                            x=x+mFixLineXStep, y=y+mFixLineYStep) {
                            sendFixedPoint(x+mFixLineXStep, y+mFixLineYStep, ACTION_MOVE);
                            usleep(mFixLineInterval*1000);
                        }
                        sendFixedPoint(x, y, ACTION_UP);
                        y0 += FIX_LINE_OFFSET;
                        sleep(1);
                    } else {
                        mEventQueue->enqueue(EVENT_TYPE_CLEAR);
                        sleep(1);
                        y0 = mLayout.top + FIX_LINE_OFFSET;
                    }
                } mMutex->unlock();
            }
        }
        LOG(DEBUG) << "Test thread stop";
    });
}

void AutoTestThread::stop() {
    mStop = true;
    if (mTestThread.joinable()) {
        mTestThread.join();
    }
}

};
