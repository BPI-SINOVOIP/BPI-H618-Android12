#pragma once

#include <thread>
#include <utils/RefBase.h>
#include "EventQueue.h"

namespace android {

class EventHandler : public virtual RefBase {

public:

    EventHandler(sp<EventQueue> queue);
    ~EventHandler();
    void start();
    void stop();


private:
    void OnInputDetected(int key_code);
    int OnInputEvent(int fd, uint32_t epevents);

    std::thread mInputThread;
    bool mStop;
    sp<EventQueue> mEventQueue;
};

};//namespace android


