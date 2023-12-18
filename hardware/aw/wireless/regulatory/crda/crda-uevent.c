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

#define LOG_TAG "crda-uevent"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>
#include <log/log.h>
#include <pthread.h>

#define UEVENT_MSG_LEN  1024

struct uevent {
    const char *action;
    const char *devpath;
    const char *subsystem;
    const char *country;
};

typedef void (*uevent_cb)(struct uevent *event);

static int uevent_init();
static int uevent_next_event(uevent_cb cb);
extern int crda(int argc, char **argv);

static int fd = -1;

/* Returns 0 on failure, 1 on success */
int uevent_init()
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    fd = s;
    return (fd > 0);
}

static void parse_event(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->devpath = "";
    uevent->subsystem = "";
    uevent->country = "";

    while (*msg) {
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            uevent->devpath = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if (!strncmp(msg, "COUNTRY=", 8)) {
            msg += 8;
            uevent->country = msg;
        }

        /* advance to after the next \0 */
        while (*msg++)
            ;
    }
}

static int uevent_next_event(uevent_cb cb)
{
    struct pollfd fds;
    struct uevent event;
    char buffer[UEVENT_MSG_LEN+2];
    int count,nr;

    while (1) {
        fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;

        nr = poll(&fds, 1, -1);

        if(nr > 0 && ((fds.revents & POLLIN) == POLLIN)) {
            memset(buffer, 0, UEVENT_MSG_LEN+2);
            count = recv(fd, buffer, UEVENT_MSG_LEN+2, 0);
            if (count > 0) {
               parse_event(buffer, &event);
               (*cb)(&event);
            }
        }
    }
    return 0;
}

/* uevent callback function
 * KERNEL=="regulatory*", ACTION=="change", SUBSYSTEM=="platform", RUN+="$(SBINDIR)crda"
 */
static void on_uevent(struct uevent *event)
{
    int ret = 0;
    if ((strncmp(event->action, "change", 6) == 0) &&
        (strncmp(event->devpath, "/devices/platform/regulatory", 28) == 0) &&
        (strncmp(event->subsystem, "platform", 8) == 0) &&
        (strlen(event->country) == 2)) {
        ALOGD("crda received kernel request for reg [%s]", event->country);
        setenv("COUNTRY", event->country, 1);
        ret = crda(1, NULL);
        unsetenv("COUNTRY");
        ALOGD("crda run %s(%d)", ret ? "fail" : "success", ret);
    }
}

static void *uevent_monitor_thread(void *param)
{
    ALOGI("crda uevent monitor thread run");

    uevent_init();
    uevent_next_event(on_uevent);

    return param;
}

int main(int argc __unused, char **argv __unused)
{
    int ret;
    pthread_t pid;
    pthread_attr_t attr;

    ret = pthread_attr_init (&attr);
    if (ret != 0) {
        ALOGE("err: pthread_attr_init failed err=%s", strerror(ret));
        return ret;
    }

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
        ALOGE("err: pthread_attr_setdetachstate failed err=%s", strerror(ret));
        return ret;
    }

    ret = pthread_create(&pid, &attr, uevent_monitor_thread, NULL);
    if (ret) {
        ALOGE("err: pthread_create failed, ret=%d\n", ret);
        return ret;
    }

    while (true) {
        sleep(UINT32_MAX);
    }
    return 0;
}
