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

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#include "HdmiCecHal.h"
#include "uniquefd.h"

namespace sunxi {

static int _debug_tags = 0;
static void debugUpdateTags()
{
    static struct timeval previous;
    struct timeval current, interval;

    gettimeofday(&current, NULL);
    timersub(&current, &previous, &interval);

    if (interval.tv_sec > 1) {
        previous = current;
        _debug_tags = property_get_int32("hdmi.cec.debug", 0);
    }
}

static inline HdmiCecHal *getHal(const struct hdmi_cec_device *dev) {
    return const_cast<HdmiCecHal*>(static_cast<const HdmiCecHal*>(dev));
}

static int add_logical_address_hook(
        const struct hdmi_cec_device* dev, cec_logical_address_t addr)
{
    HdmiCecHal* cechal = getHal(dev);
    return cechal->addLogicalAddress(addr);
}

static void clear_logical_address_hook(const struct hdmi_cec_device* dev)
{
    HdmiCecHal* cechal = getHal(dev);
    cechal->clearLogicalAddress();
}

static int get_physical_address_hook(
        const struct hdmi_cec_device* dev, uint16_t* addr)
{
    HdmiCecHal* cechal = getHal(dev);
    return cechal->getPhysicalAddress(addr);
}

static int send_message_hook(
        const struct hdmi_cec_device* dev, const cec_message_t* msg)
{
    HdmiCecHal* cechal = getHal(dev);
    return cechal->sendMessage(msg);
}

static void register_event_callback_hook(
        const struct hdmi_cec_device* dev, event_callback_t callback, void* arg)
{
    HdmiCecHal* cechal = getHal(dev);
    cechal->registerEventCallback(callback, arg);
}

static void get_version_hook(const struct hdmi_cec_device* dev, int* version)
{
    HdmiCecHal* cechal = getHal(dev);
    cechal->getVersion(version);
}

static void get_vendor_id_hook(const struct hdmi_cec_device* dev, uint32_t* vendor_id)
{
    HdmiCecHal* cechal = getHal(dev);
    cechal->getVendorId(vendor_id);
}

static void get_port_info_hook(const struct hdmi_cec_device* dev,
        struct hdmi_port_info* list[], int* total)
{
    HdmiCecHal* cechal = getHal(dev);
    cechal->getPortInfo(list, total);
}

static void set_option_hook(const struct hdmi_cec_device* dev, int flag, int value)
{
    HdmiCecHal* cechal = getHal(dev);
    cechal->setOption(flag, value);
}

static void set_audio_return_channel_hook(
        const struct hdmi_cec_device* dev, int port_id, int flag)
{
    ALOGD("set audio return channel, port_id=%d, flag=0x%08x\n", port_id, flag);
}

static int is_connected_hook(const struct hdmi_cec_device* dev, int port_id)
{
    HdmiCecHal* cechal = getHal(dev);
    return cechal->getConnectState(port_id);
}

int HdmiCecHal::open(
        const struct hw_module_t *module,
        const char *name, struct hw_device_t **device)
{
    if (strcmp(name, HDMI_CEC_HARDWARE_INTERFACE)) {
        ALOGE("Invalid module name: %s", name);
        return -EINVAL;
    }

    std::unique_ptr<HdmiCecHal> cechal(new HdmiCecHal());
    if (!cechal) {
        ALOGE("Failed to allocate CEC Hal");
        return -ENOMEM;
    }

    cechal->common.module = const_cast<hw_module_t *>(module);
    *device = &cechal->common;
    cechal.release();
    ALOGD("Loading: sunxi HDMI CEC module");
    return 0;
}

int HdmiCecHal::close(hw_device_t *dev)
{
    ALOGI("HWComposer module close");
    return 0;
}

HdmiCecHal::HdmiCecHal()
    : mDevice(nullptr),
      mSystemControl(true),
      mConnectedState(HDMI_NOT_CONNECTED),
      mLogicalAddress(CEC_ADDR_BROADCAST),
      mPhysicalAddress(0),
      mEventCallbackInfo(nullptr)
{
    common.tag     = HARDWARE_DEVICE_TAG;
    common.version = HDMI_CEC_DEVICE_API_VERSION_1_0;
    common.close   = close;

    add_logical_address      = add_logical_address_hook;
    clear_logical_address    = clear_logical_address_hook;
    get_physical_address     = get_physical_address_hook;
    send_message             = send_message_hook;
    register_event_callback  = register_event_callback_hook;
    get_version              = get_version_hook;
    get_vendor_id            = get_vendor_id_hook;
    get_port_info            = get_port_info_hook;
    set_option               = set_option_hook;
    set_audio_return_channel = set_audio_return_channel_hook;
    is_connected             = is_connected_hook;

    initializeDevice();
}

HdmiCecHal::~HdmiCecHal() = default;

void HdmiCecHal::initializeDevice()
{
    mDevice = createHdmiCecDevice();

    //Initialize ports
    mPortInfo = new hdmi_port_info[NUM_HDMI_PORTS];
    memset(mPortInfo, 0, sizeof(hdmi_port_info) * NUM_HDMI_PORTS);
    for (int i = 0; i < NUM_HDMI_PORTS; i++) {
        mPortInfo[0].type = HDMI_OUTPUT;
        mPortInfo[0].arc_supported = 0;
        mPortInfo[0].cec_supported = 1;
        mPortInfo[0].port_id = HDMI_PORTID_START + i;
        mPortInfo[0].physical_address = 0x1000;
    }

    mEventThread = std::thread(&HdmiCecHal::threadLoop, this);
    pthread_setname_np(mEventThread.native_handle(), "EventThread");
}

int read_from_file(const char *path, char *buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        ALOGE("Could not open '%s', %s(%d)", path, strerror(errno), errno);
        return -errno;
    }
    ssize_t count = read(fd, buf, size - 1);
    if (count > 0)
        buf[count] = '\0';
    else
        buf[0] = '\0';
    close(fd);
    return count;
}

void HdmiCecHal::updateConnectedState()
{
#define HDMI_SWITCH_STATE_FILE "/sys/class/extcon/extcon0/state"

    int connected = 0;
    char buf[32] = {0};
    if (read_from_file(HDMI_SWITCH_STATE_FILE, buf, 32) > 0) {
        connected = strstr(buf, "HDMI=1") != NULL ? HDMI_CONNECTED : HDMI_NOT_CONNECTED;
    }

    if (connected != mConnectedState) {
        mConnectedState = connected;
        hdmi_event_t event;
        event.type = HDMI_EVENT_HOT_PLUG;
        event.dev = this;
        event.hotplug.port_id = HDMI_DEFAULT_PORT;
        event.hotplug.connected = connected;

        std::lock_guard<std::mutex> lock(mEventCallbackMutex);
        if (mEventCallbackInfo) {
            mEventCallbackInfo->function(&event, mEventCallbackInfo->arg);
            ALOGD("update hdmi connected: %d", mConnectedState);
        }
    }
}

void HdmiCecHal::threadLoop()
{
    ALOGI("HDMI-CEC EventThread start");

    while (true) {
        uniquefd ufd(mDevice->getPollingFd());
        struct pollfd fds[1];
        fds[0].fd = ufd.get();
        fds[0].events = POLLIN;
        int ret = poll(fds, 1, CEC_POLL_TIMEOUT);

        if (ret < 0) {
            ALOGE("poll error: %s", strerror(errno));
            ALOGE("HDMI-CEC EventThread terminate");
            return;
        } else if (ret > 0) {
            cec_message_t msg;
            if (mDevice->receive(&msg) == 0) {
                onMessageReceived(msg);
            }
        }

        updateConnectedState();
        debugUpdateTags();
    }
}

void HdmiCecHal::onMessageReceived(const cec_message_t& msg)
{
    std::lock_guard<std::mutex> lock(mEventCallbackMutex);
    if (mEventCallbackInfo && mSystemControl) {
        hdmi_event_t event;
        event.type = HDMI_EVENT_CEC_MESSAGE;
        event.dev = this;
        event.cec = msg;
        mEventCallbackInfo->function(&event, mEventCallbackInfo->arg);
    }
    if (_debug_tags & 0x01) {
        dumpMessage(msg, "<");
    }
}

void HdmiCecHal::dumpMessage(const cec_message_t& msg, const char *prefix)
{
    char buf[256] = { 0 };
    char *p = buf;
    p += sprintf(p, "%s cec-msg [%02x --> %02x]:",
            prefix, msg.initiator, msg.destination);
    for (size_t i = 0; i < msg.length; i++) {
        p += sprintf(p, " %02x", msg.body[i]);
    }
    ALOGD("%s", buf);
}

void HdmiCecHal::setOption(int flag, int value)
{
    switch (flag) {
        case HDMI_OPTION_SYSTEM_CEC_CONTROL:
            mSystemControl = !!value;
            break;
        default:
            ALOGD("setOption, flag=0x%04x, value=0x%08x\n", flag, value);
            break;
    };
}

int HdmiCecHal::addLogicalAddress(cec_logical_address_t addr)
{
    mLogicalAddress = addr;
    mDevice->setAddress(mLogicalAddress);
    ALOGD("addLogicalAddress (%d)", addr);
    return 0;
}

void HdmiCecHal::clearLogicalAddress()
{
    mLogicalAddress = CEC_ADDR_BROADCAST;
    mDevice->setAddress(mLogicalAddress);
    ALOGD("reset logical address to broadcast");
}

int HdmiCecHal::getPhysicalAddress(uint16_t* addr)
{
    mPhysicalAddress = mDevice->getPhysicalAddress();
    *addr = mPhysicalAddress;
    ALOGD("cec physical address: %d.%d.%d.%d",
            (mPhysicalAddress >> 12) & 0x0f,
            (mPhysicalAddress >>  8) & 0x0f,
            (mPhysicalAddress >>  4) & 0x0f,
            (mPhysicalAddress >>  0) & 0x0f);
    return 0;
}

int HdmiCecHal::sendMessage(const cec_message_t* msg)
{
    int ret = mDevice->transmit(msg);
    if (_debug_tags & 0x01) {
        dumpMessage(*msg, ">");
        ALOGD("sendMessage ret [%d]", ret);
    }
    return ret;
}

void HdmiCecHal::registerEventCallback(event_callback_t callback, void* arg)
{
    std::lock_guard<std::mutex> lock(mEventCallbackMutex);
    if (mEventCallbackInfo != nullptr) {
        ALOGD("override event callback!");
    }
    mEventCallbackInfo = std::make_unique<EventCallbackInfo>();
    mEventCallbackInfo->function = callback;
    mEventCallbackInfo->arg = arg;
    {
        hdmi_event_t event;
        event.type = HDMI_EVENT_HOT_PLUG;
        event.dev = this;
        event.hotplug.port_id = HDMI_DEFAULT_PORT;
        event.hotplug.connected = mConnectedState;
        if (mEventCallbackInfo) {
            mEventCallbackInfo->function(&event, mEventCallbackInfo->arg);
        }
    }
}

void HdmiCecHal::getVersion(int* version)
{
    *version = CEC_VERSION_1_4;
}

void HdmiCecHal::getVendorId(uint32_t* vendor_id)
{
    *vendor_id = 0x769394;
}

void HdmiCecHal::getPortInfo(struct hdmi_port_info* list[], int* total)
{
    *list = mPortInfo;
    *total = NUM_HDMI_PORTS;
}

int HdmiCecHal::getConnectState(int port_id)
{
    return mConnectedState;
}

} // namespace sunxi

static struct hw_module_methods_t cec_module_methods = {
    .open = sunxi::HdmiCecHal::open,
};

hw_module_t HAL_MODULE_INFO_SYM = {
    .tag      = HARDWARE_MODULE_TAG,
    .module_api_version = HARDWARE_MODULE_API_VERSION(1, 0),
    .id       = HDMI_CEC_HARDWARE_MODULE_ID,
    .name     = "Allwinner hdmi cec hal",
    .author   = "Allwinner",
    .methods  = &cec_module_methods,
    .dso      = nullptr,
    .reserved = {0},
};

