#define LOG_NDEBUG 0
#define LOG_TAG "EventQueue"

#include <android-base/logging.h>

#include "EventQueue.h"

namespace android {

EventQueue::EventQueue() {
    queueMutex = new Mutex();
    dequeueCondition = new Condition();
    memset(&mInputEvent, 0, sizeof(mInputEvent));
}

EventQueue::~EventQueue() {
    delete dequeueCondition;
    delete queueMutex;
}

void EventQueue::enqueue(const InputData& event) {
    //LOG(DEBUG) << "mQueue.push " << event.x << "," << event.y;
    queueMutex->lock(); {
        mQueue.push(event);
        dequeueCondition->signal();
    } queueMutex->unlock();
}

void EventQueue::enqueue(int eventType, void* data) {
    mInputEvent.eventType = eventType;
    mInputEvent.data = data;
    enqueue(mInputEvent);
}

void EventQueue::dequeue_all(std::queue<InputData>& empty) {
    queueMutex->lock(); {
        if(mQueue.empty())
            dequeueCondition->wait(*queueMutex);
        mQueue.swap(empty);
    } queueMutex->unlock();
}

}; // namespace android
