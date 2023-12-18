#pragma once

#include<queue>
#include <utils/Condition.h>
#include <utils/RefBase.h>
#include "CommonDefs.h"

namespace android {

class EventQueue : public virtual RefBase {

public:

    EventQueue();
    ~EventQueue();
    void enqueue(const InputData& event);
    void enqueue(int eventType, void* data = nullptr);
    void dequeue_all(std::queue<InputData>& empty);

private:
    std::queue<InputData> mQueue;
    Mutex *queueMutex;
    Condition *dequeueCondition;
    InputData mInputEvent;
};

};//namespace android


