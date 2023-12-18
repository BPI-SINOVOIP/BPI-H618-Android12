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

#ifndef SUNXI_CEC_DEVICE_H
#define SUNXI_CEC_DEVICE_H

#include <memory>
#include <hardware/hdmi_cec.h>

namespace sunxi {

class ICecDevice {
public:
    virtual void setAddress(int addr) = 0;
    virtual uint16_t getPhysicalAddress() = 0;
    virtual int transmit(const cec_message_t *msg) = 0;
    virtual int receive(cec_message_t *msg) = 0;
    virtual int getPollingFd() = 0;

    virtual ~ICecDevice() = default;
};

std::unique_ptr<ICecDevice> createHdmiCecDevice();

}
#endif
