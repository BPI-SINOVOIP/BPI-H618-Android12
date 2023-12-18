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

#include "debug.h"
#include "mq_thread.h"

MessageBase::MessageBase()
    : MessageHandler() {
}

MessageBase::~MessageBase() {
}

void MessageBase::handleMessage(const android::Message&) {
    this->handler();
    android::Mutex::Autolock _l(mLock);
    mCondition.broadcast();
}

MessageQueueThread::MessageQueueThread() {
    mLooper = android::Looper::prepare(
            android::Looper::PREPARE_ALLOW_NON_CALLBACKS);
}

MessageQueueThread::~MessageQueueThread() {
}

int MessageQueueThread::postMessage(
        const android::sp<MessageBase>& messageHandler, nsecs_t delay)
{
    const android::Message dummy;
    if (delay > 0) {
        mLooper->sendMessageDelayed(delay, messageHandler, dummy);
    } else {
        mLooper->sendMessage(messageHandler, dummy);
    }
    return 0;
}

void MessageQueueThread::waitMessage()
{
    do {
        android::IPCThreadState::self()->flushCommands();
        int32_t ret = mLooper->pollOnce(-1);
        switch (ret) {
        case android::Looper::POLL_WAKE:
        case android::Looper::POLL_CALLBACK:
            continue;
        case android::Looper::POLL_ERROR:
            dd_error("Looper: poll error");
            continue;
        case android::Looper::POLL_TIMEOUT:
            continue;
        default:
            dd_error("Looper: pollOnce() return unknow status (%d)", ret);
            continue;
        }
    } while (true);
}

bool MessageQueueThread::threadLoop() {
    waitMessage();
    return true;
}

