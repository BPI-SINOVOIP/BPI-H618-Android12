#define LOG_NDEBUG 0
#define LOG_TAG "DrawThread"
#define ATRACE_TAG ATRACE_TAG_ALWAYS

#include <android-base/logging.h>

#include <fcntl.h>

#include <sys/resource.h>
#include <sys/prctl.h>

#include <SkPath.h>
#include <cutils/trace.h>

#include "DisplayConfigHal.h"
#include "DrawThread.h"
#include "EventQueue.h"

namespace android
{

DrawThread::DrawThread(sp<EventQueue> queue, sp<SkRenderer> renderer, const Rect& r) :
        mEventQueue(queue),
        mSkRenderer(renderer),
        mScreenRect(r),
        mStrokeWidth(STROKE_WIDTH_MAX) {
    LOG(DEBUG) << "ScreenRect: [" << r.left << ", " << r.top << ", " << r.right << ", " << r.bottom << "]";
}

DrawThread::~DrawThread() {
    LOG(DEBUG) << "~DrawThread";

    if (!mStop) {
        mEventQueue->enqueue(EVENT_TYPE_STOP);
        stop();
    }
}

void DrawThread::start() {
    mStop = false;
    mLastX = mLastY = -1;
    mDrawThread = std::thread([this]() {
        LOG(DEBUG) << "Draw thread start";
        setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);
        prctl(PR_SET_NAME,"DrawThread");
        while (!this->mStop) {
            this->processInputData();
        }
        LOG(DEBUG) << "Draw thread stop";
    });
}

void DrawThread::stop() {
    mStop = true;
    if (mDrawThread.joinable()) {
        mDrawThread.join();
    }
}

void DrawThread::processInputData() {
    ATRACE_BEGIN("processInputData");

    bool needUpdateRegion = false;
    bool isRubber = false;
    mEventQueue->dequeue_all(mPointsQueue);
    while (!mPointsQueue.empty()) {
        InputData point = mPointsQueue.front();
        mPointsQueue.pop();
        //LOG(DEBUG) << "mQueue.pop " << point.eventType << ":" << point.x << "," << point.y;
        if (point.eventType == EVENT_TYPE_STOP) {
            mStop = true;
            needUpdateRegion = false;
            break;
        } else if (point.eventType == EVENT_TYPE_FOCUS) {
            bool focus = *((bool *)point.data);
            mWindowFocus = focus;
            LOG(DEBUG) << "window focus: " << mWindowFocus;
            if (!mWindowFocus) {
                mFirstUpdatePath = true;
                LOG(DEBUG) << "window lost focus, reset";
            }
            needUpdateRegion = false;
            continue;
        } else if (point.eventType == EVENT_TYPE_CLEAR) {
            mSkRenderer->clearCanvas();
            if (mListener != nullptr) {
                mListener->onUpdateRegion(mScreenRect, true);
                mListener->onUpdateBuffer();
            }
            needUpdateRegion = false;
            continue;
        } else if (point.eventType == EVENT_TYPE_REFRESH) {
            if (mListener != nullptr) {
                mListener->onUpdateRegion(mScreenRect);
            }
            needUpdateRegion = false;
            continue;
        } else if (point.eventType == EVENT_TYPE_BACKGROUND) {
            mSkRenderer->setBackground((SkBitmap*)point.data);
            if (mListener != nullptr) {
                mListener->onUpdateRegion(mScreenRect, true);
                mListener->onUpdateBuffer();
            }
            needUpdateRegion = false;
            continue;
        } else {
            ATRACE_BEGIN("drawPoint");
            isRubber = point.eventType == EVENT_TYPE_RUBBER;
            needUpdateRegion |= drawPoint(point);
            ATRACE_END();
        }
    }

    if (isRubber) {
        mSkRenderer->redrawRubberRegion(mRegion.getBounds(), 0, 0);
    }
    if (needUpdateRegion) doUpdateRegion();
    ATRACE_END();
}

bool DrawThread::drawPoint(const InputData& event) {
    //LOG(DEBUG) << "\tdrawPoint type: " << event.type << ", (\t" << event.x << "\t, \t" << event.y << "\t), eventType:" << event.eventType;
    nsecs_t startTime;

    bool needUpdateRegion = false;
    SkRect skRect;
    switch(event.type) {
        case ACTION_DOWN:
            mPath.reset();
            mPath.moveTo(event.x, event.y);
            break;
        case ACTION_MOVE:
            if (mLastX == event.x && mLastY == event.y)
                break;
            ATRACE_BEGIN("DrawPath");
            if (mDebugDrawTime) {
                startTime = systemTime(SYSTEM_TIME_MONOTONIC);
            }
            mPath.lineTo(event.x, event.y);
            needUpdateRegion = true;
            if (event.eventType == EVENT_TYPE_PEN || event.eventType == EVENT_TYPE_TOUCH) {
                mSkRenderer->setStrokeWidth(mStrokeWidth);
                mSkRenderer->drawPath(mPath, skRect);
            } else if (event.eventType == EVENT_TYPE_RUBBER) {
                mSkRenderer->drawRubberPath(mPath, skRect);
            }
            mRegion.orSelf(Rect(skRect.left(), skRect.top(), skRect.right(), skRect.bottom()));
            mPath.reset();
            mPath.moveTo(event.x, event.y);
            if (mDebugDrawTime) {
                nsecs_t endTime = systemTime(SYSTEM_TIME_MONOTONIC);
                double interval = (endTime - startTime)/1000000;
                if (interval > 1) {
                    LOG(DEBUG) << "drawPath duration !!!!!!!"<< interval << "ms";
                }
            }
            ATRACE_END();
            break;
        case ACTION_UP:
            if (mListener != nullptr) {
                mListener->onUpdateBuffer();
            }
            break;
    }
    mLastX = event.x;
    mLastY = event.y;
    mStrokeWidth = getStrokeWitdh(event.pressure);
    return needUpdateRegion;
}

void DrawThread::doUpdateRegion() {
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    //if (now - mUpdateRegionTime < 12*1000000) return;
    //LOG(DEBUG) << "doUpdateRegion duration " << (now - mUpdateRegionTime)/1000000 << "ms";
    ATRACE_BEGIN("updateDamageRegion");

    if (mListener != nullptr) {
        /*
         * HWC does not refresh the non-handwriting layer in handwriting mode,
         * so that it can't enter handwriting mode too early, for example, when
         * DrawThread starting, which lead to Buttons not show.
         */
        if (mFirstUpdatePath && mWindowFocus) {
            LOG(DEBUG) << "doUpdateRegion enter handwritten mode";
            DisplayConfigHal::getInstance()->setRefreshMode(IEinkMode::DU, true);
            DisplayConfigHal::getInstance()->setHandwrittenArea(mScreenRect);
            mFirstUpdatePath = false;
        }

        Rect dirtyRect;
        mRegion.getBounds().intersect(mScreenRect, &dirtyRect);
        if (RET_OK == mListener->onUpdateRegion(dirtyRect)) {
           mRegion.clear();
        }
    }
    ATRACE_END();
    mUpdateRegionTime = now;
}

}; // namespace android
