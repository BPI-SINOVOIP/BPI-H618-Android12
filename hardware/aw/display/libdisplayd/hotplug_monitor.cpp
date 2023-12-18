/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include <cutils/log.h>
#include <sysutils/NetlinkEvent.h>
#include "hotplug_monitor.h"

HotplugMonitor::HotplugMonitor(const Callback *cb, int sockfd)
  : NetlinkListener(sockfd, NetlinkListener::NETLINK_FORMAT_ASCII),
    mSocket(sockfd),
    mCallback(cb)
{
    this->startListener();
}

HotplugMonitor::~HotplugMonitor()
{
    this->stopListener();
    close(mSocket);
}

void
HotplugMonitor::onEvent(NetlinkEvent *event)
{
    const char *subsys = event->getSubsystem();
    if (!subsys) {
        ALOGE("No subsystem found in netlink event");
        return;
    }
#ifndef SUNXI_USE_LEGACY_KERNEL
    const char* prefix[] = {"extcon", "NAME", "STATE"};
#else
    const char* prefix[] = {"switch", "SWITCH_NAME", "SWITCH_STATE"};
#endif

    if (!strcmp(subsys, prefix[0])) {
        if (event->getAction() == NetlinkEvent::Action::kChange) {
            const char *name  = event->findParam(prefix[1]);
            const char *state = event->findParam(prefix[2]);
            if((NULL == name) || (NULL == state)) {
                ALOGE("null point: name=%s, state=%s.", name, state);
                return;
            }
            ALOGD("hotplug change: name=%s, state=%s ", name, state);
            bool plug = (strstr(state, "1") != NULL);
            notifyHotplugChanged(name, plug);
        } else {
            event->dump();
        }
    }
    return;
}

#define NEMLEN(_array) \
    (sizeof(_array) / sizeof(_array[0]))

static int name2type(const char *name)
{
    struct deviceTypeMap {
        const char *name; const int type;
    };
    const deviceTypeMap nameTypeArray[] = {
        {"hdmi", 4},
        {"cvbs", 2},
    };

    for (size_t i = 0; i < NEMLEN(nameTypeArray); i++) {
        if (strcmp(name, nameTypeArray[i].name) == 0)
            return nameTypeArray[i].type;
    }
    return -1;
}

void
HotplugMonitor::notifyHotplugChanged(const char *name, int connected)
{
    ALOGD("HotplugMonitor: %s connected=%d", name, connected);
    if (mCallback)
        mCallback->hotplug(mCallback, name2type(name), connected);
    return;
}

int
HotplugMonitor::createUeventSocket(void)
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int on = 1;
    int sockfd;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = ~getpid();
    addr.nl_groups = 0xffffffff;

    sockfd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(sockfd < 0) {
        ALOGE("create socket error: %s(%d)", strerror(errno), errno);
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz)) < 0) {
        ALOGE("setsockopt(SO_RCVBUF) error: %s(%d)", strerror(errno), errno);
        close(sockfd);
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
        ALOGE("setsockopt(SO_PASSCRED) error: %s(%d)", strerror(errno), errno);
        close(sockfd);
        return -1;
    }
    if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        ALOGE("bind error: %s(%d)", strerror(errno), errno);
        close(sockfd);
        return 0;
    }

    return sockfd;
}

