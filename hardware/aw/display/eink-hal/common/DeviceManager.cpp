/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <map>
#include <set>
#include <pthread.h>
#include <hardware/hwcomposer2.h>

#include "Debug.h"
#include "DeviceManager.h"

using namespace sunxi;

class DeviceManager::EventQueue {
public:
    EventQueue() = default;

    class Event;
    void enqueueEvent(const Event& event);
    Event waitForEvent();

    class Event {
    public:
        enum {
            DEVICE_CHANGED = 1,
        };
        int mEvent;
        std::vector<ConnectedInfo> mConnectedInfo;

        Event() = default;
    };

private:
    std::queue<Event> mEvents;
    mutable std::mutex mMutex;
    mutable std::condition_variable mCondition;
};

using Event = DeviceManager::EventQueue::Event;

void DeviceManager::EventQueue::enqueueEvent(const Event& event)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mEvents.emplace(event);
    mCondition.notify_all();
    DLOGD("enqueueEvent: event %d", event.mEvent);
}

Event DeviceManager::EventQueue::waitForEvent()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mEvents.empty())
        mCondition.wait(lock);

    Event event = mEvents.front();
    mEvents.pop();
    return event;
}

DeviceManager::DeviceManager()
    : mEventQueue(new EventQueue()),
      mRunning(false),
      mEventThread(),
      mHotplugCallback(nullptr),
      mLogicalDisplays(),
      mDeviceHubs() { }

DeviceManager::~DeviceManager() { }

void DeviceManager::startEventThread()
{
    mRunning.store(true);
    mEventThread = std::thread(&DeviceManager::threadLoop, this);
    pthread_setname_np(mEventThread.native_handle(), "EventThread");
}

void DeviceManager::stopEventThread()
{
    mRunning.store(false);
    mEventQueue->enqueueEvent(EventQueue::Event());
    mEventThread.join();
}

void DeviceManager::registerHotplugCallback(HotplugCallback cb)
{
    mHotplugCallback = cb;
}

void DeviceManager::onDeviceChanged(std::vector<ConnectedInfo>& cinfo)
{
    EventQueue::Event event;
    event.mEvent = EventQueue::EventQueue::Event::DEVICE_CHANGED;
    event.mConnectedInfo = cinfo;
    mEventQueue->enqueueEvent(event);
}

std::shared_ptr<DeviceBase> DeviceManager::getDeviceByDisplayId(int32_t id)
{
    std::unique_lock<std::mutex> lock(mDisplayLock);
    if (mLogicalDisplays.count(id) == 0) {
        DLOGW("Display(%d) has not connected yet!", id);
        return nullptr;
    }
    return mLogicalDisplays[id];
}

DeviceConnection DeviceManager::createDeviceConnection(int32_t id)
{
    std::unique_lock<std::mutex> lock(mDisplayLock);

    if (mDeviceHubs.count(id)) {
        mDeviceHubs.erase(id);
        DLOGW("Erase duplicate DeviceHub (Display id: %d)", id);
    }

    DeviceConnection connection;
    if (mLogicalDisplays.count(id)) {
        std::shared_ptr<DeviceBase> device = mLogicalDisplays[id];
        std::shared_ptr<DeviceHub> hub = std::make_shared<DeviceHub>(id, device);
        mDeviceHubs.emplace(id, hub);
        connection.setDeviceHub(hub);
    }

    return connection;
}

void DeviceManager::disconnect(int32_t id)
{
    std::unique_lock<std::mutex> lock(mDisplayLock);
    mDeviceHubs.erase(id);
}

void DeviceManager::deviceChangedEventProcess(std::vector<ConnectedInfo>& cinfo)
{
    // every acceptable device mapping should have one primary display.
    int primaryDisplayCount = 0;
    std::for_each(cinfo.begin(), cinfo.end(),
            [&primaryDisplayCount](const ConnectedInfo& c) {
                if (c.displayId == HWC_DISPLAY_PRIMARY)
                    primaryDisplayCount++;
            });
    if (primaryDisplayCount != 1) {
        DLOGE("Primary display count must be ONE! but (%d)", primaryDisplayCount);
        return;
    }

    std::vector<std::pair<int32_t, bool>> pendingHotplugs;
    { // mDisplayLock scope
        std::unique_lock<std::mutex> lock(mDisplayLock);

        std::set<int32_t> connectedDisplays;
        std::for_each(mLogicalDisplays.begin(), mLogicalDisplays.end(),
                [&connectedDisplays](const auto& item) {
                connectedDisplays.insert(item.first);
                });

        for (auto iter = cinfo.begin(); iter != cinfo.end(); ++iter) {
            int32_t displayId = (*iter).displayId;
            int32_t connected = (*iter).connected;
            std::shared_ptr<DeviceBase>& device = (*iter).device;

            if (connectedDisplays.count(displayId)) {
                if (!connected) {
                    // The device was plugin before, but now it is not,
                    // Issue a hotplug out event.
                    pendingHotplugs.push_back(std::make_pair(displayId, false));
                    DLOGW("Display.%d (%s) disconnect",
                            displayId, mLogicalDisplays[displayId]->name());
                    mLogicalDisplays.erase(displayId);
                } else {
                    mLogicalDisplays.insert_or_assign(displayId, device);
                }
            } else {
                // new connect device
                if (connected) {
                    mLogicalDisplays.insert_or_assign(displayId, device);
                    pendingHotplugs.push_back(std::make_pair(displayId, true));
                    DLOGW("Display.%d (%s) connect",
                            displayId, mLogicalDisplays[displayId]->name());
                }
            }
        }

        // update new mapping to every connection
        populateDeviceHub(cinfo);

    } // endof mDisplayLock scope

    for (auto& hotplug : pendingHotplugs) {
        mHotplugCallback(hotplug.first, hotplug.second);
    }
}

void DeviceManager::populateDeviceHub(std::vector<ConnectedInfo>& cinfo)
{
    for (int i = cinfo.size() - 1; i >= 0; i--) {
        ConnectedInfo& info = cinfo[i];
        int32_t displayId = info.displayId;
        if (mDeviceHubs.count(displayId)) {
            std::shared_ptr<DeviceHub>& hub = mDeviceHubs[displayId];
            if (hub->getDevice() != info.device) {
                DLOGW("Display.%d device changed: %s ==> %s",
                        displayId, hub->getDevice()->name(),
                        info.device->name());
                hub->onDeviceChanged(info.device);
            }
        }
    }
}

void DeviceManager::threadLoop()
{
    DLOGI("EventThread start");
    while (mRunning.load()) {
        Event event = mEventQueue->waitForEvent();

        switch (event.mEvent) {
        case Event::DEVICE_CHANGED:
            deviceChangedEventProcess(event.mConnectedInfo);
            break;
        default:
            DLOGW("Not handle device event(%d)", event.mEvent);
            break;
        }
    }
}
