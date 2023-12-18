/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MESSAGE_QUEUE_THREAD_H_
#define _MESSAGE_QUEUE_THREAD_H_

#include <utils/Thread.h>
#include <utils/Looper.h>
#include <binder/IPCThreadState.h>

class MessageBase: public android::MessageHandler {
public:
    MessageBase();
    virtual bool handler() = 0;

    void wait() const {
        android::Mutex::Autolock _l(mLock);
        mCondition.wait(mLock);
    }

protected:
    virtual ~MessageBase();

private:
    virtual void handleMessage(const android::Message& message);

    mutable android::Mutex mLock;
    mutable android::Condition mCondition;
};

class MessageQueueThread: public android::Thread {
public:
    MessageQueueThread();
   ~MessageQueueThread();

    int postMessage(const android::sp<MessageBase>& message, nsecs_t delay = 0);

private:
    void init();
    void waitMessage();
    virtual bool threadLoop();

    android::sp<android::Looper> mLooper;
};

#endif

