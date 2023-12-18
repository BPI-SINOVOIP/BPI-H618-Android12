#define LOG_NDEBUG 0
#define LOG_TAG "EventHandler"
#define ATRACE_TAG ATRACE_TAG_ALWAYS

#include <android-base/logging.h>
#include <linux/input.h>
#include <cutils/trace.h>

#include "events.h"
#include "EventHandler.h"

namespace android {

EventHandler::EventHandler(sp<EventQueue> queue) : mEventQueue(queue) {
    ev_init(std::bind(&EventHandler::OnInputEvent, this, std::placeholders::_1, std::placeholders::_2), false);
    ev_iterate_available_keys(std::bind(&EventHandler::OnInputDetected, this, std::placeholders::_1));
    ev_iterate_touch_inputs(std::bind(&EventHandler::OnInputDetected, this, std::placeholders::_1));
}

EventHandler::~EventHandler() {
    LOG(DEBUG) << "~EventHandler";

    if (!mStop) {
        stop();
    }

    ev_exit();
}

void EventHandler::start() {
    mStop = false;
    mInputThread = std::thread([this]() {
        LOG(DEBUG) << "Input thread start";
        while (!this->mStop) {
            if (!ev_wait(500)) {
                ev_dispatch();
            }
        }
        LOG(DEBUG) << "Input thread stop";
    });
}

void EventHandler::stop() {
    mStop = true;
    if (mInputThread.joinable()) {
        mInputThread.join();
    }
}

int EventHandler::OnInputEvent(int fd, uint32_t epevents) {
    struct input_event event;
    ATRACE_BEGIN("ev_get_input");
    if (ev_get_input(fd, epevents, &event) == -1) {
        return -1;
    }
    ATRACE_END();
    ATRACE_BEGIN("OnInputEvent");

    static int curTouch = -1;
    static int lastTouch = -1;
    static bool pointValid = false;
    static InputData point;
    switch (event.type) {
        case EV_SYN: {
            if (curTouch != lastTouch) {
                lastTouch = curTouch;
                if (curTouch == 1) {
                    point.type = ACTION_DOWN;
                    pointValid = true;
                    mEventQueue->enqueue(point);
                } else if(curTouch == 0){
                    point.type = ACTION_UP;
                    pointValid = false;
                    mEventQueue->enqueue(point);
                    point.x = 0;
                    point.y = 0;
                    point.pressure = 0;
                }
            } else if (curTouch == 1) {
                point.type = ACTION_MOVE;
                if (pointValid) {
                    mEventQueue->enqueue(point);
                }
            }
        } break;
        case EV_KEY: {
            if (event.code == BTN_TOOL_RUBBER && event.value == 1) {
                point.eventType = EVENT_TYPE_RUBBER;
            } else if(event.code == BTN_TOOL_PEN && event.value == 1) {
                point.eventType = EVENT_TYPE_PEN;
            } else if(event.code == BTN_TOOL_FINGER && event.value == 1) {
                point.eventType = EVENT_TYPE_TOUCH;
            } else if (event.code == BTN_TOUCH) {
                curTouch = event.value;
            }
        } break;
        case EV_ABS: {
            if (event.code == ABS_MT_POSITION_X || event.code == ABS_X) {
                point.x = event.value;
            } else if (event.code == ABS_MT_POSITION_Y || event.code == ABS_Y) {
                point.y = event.value;
            } else if (event.code == ABS_MT_PRESSURE || event.code == ABS_PRESSURE) {
                point.pressure = event.value;
            }
        } break;
        default:
            break;
    }
    ATRACE_END();
    return 0;
}

void EventHandler::OnInputDetected(int key_code) {
    if (key_code == ABS_MT_POSITION_X || key_code == ABS_MT_POSITION_Y) {
        // TODO multi touch
    } else if (key_code == ABS_X || key_code == ABS_Y) {
        // TODO single pen
    }
}

}; // namespace android
