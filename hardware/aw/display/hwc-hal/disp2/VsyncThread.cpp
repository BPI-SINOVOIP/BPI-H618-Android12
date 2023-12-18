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

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

#include "Debug.h"
#include "disputils.h"
#include "uniquefd.h"
#include "VsyncThread.h"

namespace sunxi {

VsyncThread::VsyncThread()
{
    std::unique_lock<std::mutex> lock(mMutex);
    mThread = std::thread(&VsyncThread::pollThreadLoop, this);
    pthread_setname_np(mThread.native_handle(), "VsyncThread");
    mThread.detach();
}

void VsyncThread::addEventHandler(int id, EventHandler *handler)
{
    DLOGI_IF(kTagVSync, "add event handler(%p)", handler);
    std::unique_lock<std::mutex> lock(mMutex);
    mEventHandlers.insert_or_assign(id, handler);
    mCondition.notify_all();
}

void VsyncThread::removeEventHandler(int id)
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mEventHandlers.count(id)) {
        DLOGI_IF(kTagVSync, "remove event handler(%p)", mEventHandlers[id]);
        mEventHandlers.erase(id);
    }
}

void VsyncThread::handleVsyncEvent(int disp, int64_t timestamp)
{
    DLOGI_IF(kTagVSync, "display(%d) timestamp %lld", disp, timestamp);
    std::unique_lock<std::mutex> lock(mMutex);
    if (mEventHandlers.count(disp)) {
        EventHandler *handler = mEventHandlers[disp];
        handler->onVsync(disp, timestamp);
    }
}

#define UEVENT_BUF_LEN      (4096)
#define UEVENT_VSYNC_TOKEN  "change@/devices/platform/soc/disp"
#define UEVENT_VSYNC_TOKEN2 "change@/devices/platform/soc/6000000.disp"
#define UEVENT_VSYNC_TOKEN3 "change@/devices/platform/soc@2900000/6000000.disp"

static int uevent_init();
static int uevent_next_event(int socketfd, char* buffer, int lenght);

void VsyncThread::threadLoop()
{
    DLOGI("VsyncThread start");

    uniquefd socketfd(uevent_init());
    if (socketfd.get() < 0) {
        DLOGE("VsyncThread: error exit");
        return;
    }

    while (1) {
        { // scope of the mMutex
            std::unique_lock<std::mutex> lock(mMutex);
            if (mEventHandlers.empty()) {
                mCondition.wait(lock);
            }
        }

        char ueventdat[UEVENT_BUF_LEN] = {};
        // keep last 2 zeros to ensure double 0 termination
        int length = uevent_next_event(socketfd.get(),
                ueventdat,
                (int32_t)(sizeof(ueventdat)) - 2);
        parseEvent(ueventdat, length);
    }
}

/*
 * vsync uevent:
 * change@/devices/platform/soc/disp
 * change@/devices/platform/soc/6000000.disp --> A100
 * ACTION=change
 * DEVPATH=/devices/platform/soc/disp
 * SUBSYSTEM=platform
 * VSYNC0=106380170541673
 * DRIVER=disp
 * OF_NAME=disp
 * OF_FULLNAME=/soc@01c00000/disp@01000000
 * OF_TYPE=disp
 * OF_COMPATIBLE_0=allwinner,sunxi-disp
 * OF_COMPATIBLE_N=1
 * MODALIAS=of:NdispTdispCallwinner,sunxi-disp
 * SEQNUM=17486
*/
void VsyncThread::parseEvent(const char *evdata, int length)
{
    const char *evend = evdata + length;
    const char *evbegin = evdata;
    while (evbegin < evend) {
        if (strstr(evbegin, UEVENT_VSYNC_TOKEN3)
                || strstr(evbegin, UEVENT_VSYNC_TOKEN2) || strstr(evbegin, UEVENT_VSYNC_TOKEN)) {
            const char *substr = evbegin;
            while (substr < evend) {
                const char *vsync = strstr(substr, "VSYNC");
                if (vsync) {
                    int disp = vsync[5] - '0';
                    int64_t timestamp = strtoull(vsync+7, NULL, 0);
                    handleVsyncEvent(disp, timestamp);
                    break;
                }
                substr += strlen(substr) + 1;
            }
            evbegin = substr;
        } else {
            evbegin += strlen(evbegin) + 1;
        }
    }
}

// On success, a file descriptor for the new socket is returned.
// On error, -1 is returned.
static int uevent_init()
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;
    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0) {
        int err = errno;
        DLOGE("uevent_init socket() return error: %s", strerror(err));
        return -1;
    }
    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        int err = errno;
        DLOGE("uevent_init bind() return error: %s", strerror(err));
        return -1;
    }
    return s;
}

static int uevent_next_event(int socketfd, char* buffer, int lenght)
{
    while (1) {
        struct pollfd fds;
        int nr;

        fds.fd = socketfd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);

        if(nr > 0 && fds.revents == POLLIN) {
            int count = recv(socketfd, buffer, lenght, 0);
            if (count > 0) {
                return count;
            }
        }
    }

    // won't get here
    return 0;
}

void VsyncThread::pollThreadLoop()
{
    DLOGI("polling VsyncThread start");

    int fd = open("/dev/disp", O_RDWR);
    if (fd < 0) {
        DLOGE("Open '/dev/disp' failed, %s", strerror(errno));
        return;
    }

    uniquefd ufd(fd);

    while (1) {
        struct pollfd polldata;
        polldata.fd = ufd.get();
        polldata.events = POLLIN;
        polldata.revents = 0;

        int ret = poll(&polldata, 1, -1);

        if (ret < 0) {
            DLOGE("poll error: %s", strerror(errno));
            DLOGE("VSYNC polling Thread terminate");
            return;
        } else if (ret > 0) {
            if (polldata.revents & POLLIN) {
                int32_t disp;
                int64_t timestamp;
                if (getDisplayVsyncTimestamp(&disp, &timestamp) == 0) {
                    handleVsyncEvent(disp, timestamp);
                }
            }
        }
    }
}

} // namespace suxni

