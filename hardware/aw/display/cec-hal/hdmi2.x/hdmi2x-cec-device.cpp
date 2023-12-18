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

#ifndef SUNXI_CEC_DEV_2_H
#define SUNXI_CEC_DEV_2_H

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cutils/log.h>

#include "CecDevice.h"
#include "cec-drv.h"
#include "uniquefd.h"

namespace sunxi {

#define CEC_TRANSMIT_TIMEOUT    1000

class Hdmi2xCecDevice : public ICecDevice {
public:
    Hdmi2xCecDevice();
   ~Hdmi2xCecDevice();

    void setAddress(int addr) override;
    uint16_t getPhysicalAddress() override;
    int transmit(const cec_message_t *msg) override;
    int receive(cec_message_t *msg) override;
    int getPollingFd() override;

private:
    uniquefd _devfd;
};

Hdmi2xCecDevice::Hdmi2xCecDevice()
{
    int fd = open("/dev/cec", O_RDWR);
    if (fd < 0) {
        ALOGE("open /dev/cec failed: %s", strerror(errno));
        return;
    }
    _devfd = uniquefd(fd);

    // change block mode
    int flags = fcntl(_devfd.get(), F_GETFL) & ~O_NONBLOCK;
    if (fcntl(_devfd.get(), F_SETFL, flags) == -1) {
        ALOGE("failed to clear 'O_NONBLOCK' flag: %s", strerror(errno));
    }
}

Hdmi2xCecDevice::~Hdmi2xCecDevice() { }

void Hdmi2xCecDevice::setAddress(int addr)
{
    unsigned char logical_addr = addr;
    unsigned long arg[3] = {0};
    arg[0] = (unsigned long)&logical_addr;
    if (ioctl(_devfd.get(), CEC_S_LOG_ADDR, arg) != 0) {
        ALOGE("ioctl 'CEC_S_LOG_ADDR' failed: %s", strerror(errno));
    }
}

uint16_t Hdmi2xCecDevice::getPhysicalAddress()
{
    unsigned long arg[3] = {0};
    int phyaddr;
    arg[0] = (unsigned long)&phyaddr;
    return ioctl(_devfd.get(), CEC_G_PHYS_ADDR, arg);
}

int Hdmi2xCecDevice::transmit(const cec_message_t *msg)
{
    cec_msg_t param;
    memset(&param, 0, sizeof(cec_msg_t));
    param.len = msg->length + 1;
    param.msg[0] = (msg->initiator << 4) + (msg->destination & 0x0f);
    memcpy(&param.msg[1], msg->body, msg->length);
    param.timeout = CEC_TRANSMIT_TIMEOUT;

    unsigned long arg[3] = {0};
    arg[0] = (unsigned long)&param;
    ioctl(_devfd.get(), CEC_TRANSMIT, arg);

    switch (param.tx_status) {
        case CEC_TX_STATUS_OK:   return HDMI_RESULT_SUCCESS;
        case CEC_TX_STATUS_NACK: return HDMI_RESULT_NACK;
        default: return HDMI_RESULT_FAIL;
    }
}

int Hdmi2xCecDevice::receive(cec_message_t *msg)
{
    cec_msg_t param;
    memset(&param, 0, sizeof(cec_msg_t));
    unsigned long arg[3] = { 0 };
    arg[0] = (unsigned long)&param;
    if (ioctl(_devfd.get(), CEC_RECEIVE, arg) != 0) {
        ALOGE("ioctl 'CEC_RECEIVE' failed: %s", strerror(errno));
        return -1;
    }

    if (param.len < 2) {
        ALOGD("receive invalid cec message!");
        if (param.len == 1) {
            ALOGD("receive poll message [%02x], skip!", param.msg[0]);
        }
        return -1;
    }

    memset(msg, 0, sizeof(cec_message_t));
    msg->initiator = static_cast<cec_logical_address_t>((param.msg[0] >> 4) & 0x0f);
    msg->destination = static_cast<cec_logical_address_t>(param.msg[0] & 0x0f);
    msg->length = param.len - 1;
    memcpy(&(msg->body[0]), &(param.msg[1]), msg->length);
    return 0;
}

int Hdmi2xCecDevice::getPollingFd()
{
    return dup(_devfd.get());
}

std::unique_ptr<ICecDevice> createHdmiCecDevice()
{
    std::unique_ptr<ICecDevice> device = std::make_unique<Hdmi2xCecDevice>();
    return device;
}

} // namespace sunxi
#endif
