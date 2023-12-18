/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <memory>
#include <utility>
#include <thread>
#include <hardware/hdmi_cec.h>
#include <hardware/hardware.h>

#include "CecDevice.h"

namespace sunxi {

// We support only one output port
#define NUM_HDMI_PORTS          1
#define HDMI_PORTID_START       1
#define HDMI_DEFAULT_PORT       HDMI_PORTID_START

// time in milliseconds
#define CEC_POLL_TIMEOUT        500

// definition from HdmiCecMessageBuilder.java
#define CEC_VERSION_1_3_A   (0x04)
#define CEC_VERSION_1_4     (0x05)

class HdmiCecHal: public hdmi_cec_device_t {
public:
    static int open(const struct hw_module_t *module,
                    const char *name, struct hw_device_t **dev);
    static int close(hw_device_t *dev);

    HdmiCecHal();
   ~HdmiCecHal();

    int addLogicalAddress(cec_logical_address_t addr);
    void clearLogicalAddress();
    int getPhysicalAddress(uint16_t* addr);
    int sendMessage(const cec_message_t*);
    void registerEventCallback(event_callback_t callback, void* arg);
    void getVersion(int* version);
    void getVendorId(uint32_t* vendor_id);
    void getPortInfo(struct hdmi_port_info* list[], int* total);
    void setOption(int flag, int value);
    int getConnectState(int port_id);

private:
    void initializeDevice();
    void updateConnectedState();
    void threadLoop();
    void onMessageReceived(const cec_message_t& msg);
    void dumpMessage(const cec_message_t& msg, const char *prefix);

    std::unique_ptr<ICecDevice> mDevice;
    bool mSystemControl;
    int mConnectedState;
    cec_logical_address_t mLogicalAddress;
    uint16_t mPhysicalAddress;
    hdmi_port_info *mPortInfo;

    std::mutex mEventCallbackMutex;
    struct EventCallbackInfo {
        event_callback_t function;
        void *arg;
    };
    std::unique_ptr<EventCallbackInfo> mEventCallbackInfo;

    // event thread to receive cec message
    std::thread mEventThread;
};

}
