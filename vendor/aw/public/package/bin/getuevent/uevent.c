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
#define LOG_TAG "uevent"

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
#include<unistd.h>
#include <getopt.h>

#define UEVENT_MSG_LEN  1024
#define ENV_MAX         64
#define ARRAY_SIZE(x)   sizeof(x) / sizeof(x[0])

#define UEVT_LOGD(fmt, arg...)           \
  do {                                   \
    fprintf(stdout, fmt "\n" , ##arg);   \
    ALOGD(fmt, ##arg);                   \
  } while(0)

#define UEVT_LOGE(fmt, arg...)           \
  do {                                   \
    fprintf(stderr, fmt "\n" , ##arg);   \
    ALOGE(fmt, ##arg);                   \
  } while(0)

#define UEVT_LOGW(fmt, arg...)           \
  do {                                   \
    fprintf(stderr, fmt "\n" , ##arg);   \
    ALOGW(fmt, ##arg);                   \
  } while(0)


struct uevent {
    const char *envv[ENV_MAX];
    uint32_t    envc;
    uint32_t    show;
};

struct filter_t {
    const char *action;
    const char *bus;
    const char *driver;
    const char *id;
    const char *devpath;
    const char *subsystem;
    uint32_t valid;
};

typedef void (*uevent_cb)(struct uevent *uevt);

static int uevent_init();
static int uevent_next_event(uevent_cb cb);

static int fd = -1;
static struct filter_t filter;

struct option_t {
    const struct option long_options;
    const char *help;
};

static const struct option_t option_list[] = {
    {{"action",    1, NULL, 'a'}, "define match context after 'ACTION='"},
    {{"bus",       1, NULL, 'b'}, "define match context after 'BUS='"},
    {{"driver",    1, NULL, 'd'}, "define match context after 'DRIVER='"},
    {{"id",        1, NULL, 'i'}, "define match context after 'ID='"},
    {{"devpath",   1, NULL, 'p'}, "define match context after 'DEVPATH='"},
    {{"subsystem", 1, NULL, 's'}, "define match context after 'SUBSYSTEM='"},
    {{"help",      0, NULL, 'h'}, "show this help"},
};

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

static void parse_event(const char *msg, struct uevent *uevt)
{
    uint32_t idx = 0, valid = 0;

    while (*msg) {
        uevt->envv[idx++] = msg;
        if (!strncmp(msg, "ACTION=", 7)) {
           msg += 7;
           if (filter.action && (strlen(filter.action) == strlen(msg)) &&
                  (strcmp(filter.action, msg) == 0))
               valid++;
        } else if (!strncmp(msg, "BUS=", 4)) {
           msg += 4;
           if (filter.bus && (strlen(filter.bus) == strlen(msg)) &&
                  (strcmp(filter.bus, msg) == 0))
               valid++;
        } else if (!strncmp(msg, "DRIVER=", 7)) {
           msg += 7;
           if (filter.driver && (strlen(filter.driver) == strlen(msg)) &&
                  (strcmp(filter.driver, msg) == 0))
               valid++;
        } else if (!strncmp(msg, "ID=", 3)) {
           msg += 3;
           if (filter.id && (strlen(filter.id) == strlen(msg)) &&
                  (strcmp(filter.id, msg) == 0))
               valid++;
        } else if(!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            if (filter.devpath && (strlen(filter.devpath) == strlen(msg)) &&
                  (strcmp(filter.devpath, msg) == 0))
               valid++;
        } else if(!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            if (filter.subsystem && (strlen(filter.subsystem) == strlen(msg)) &&
                  (strcmp(filter.subsystem, msg) == 0))
               valid++;
        }
        /* advance to after the next \0 */
        while (*msg++)
            ;
    }
    uevt->envv[idx] = NULL;
    uevt->envc = idx;
    uevt->show = 0;
    if (valid == filter.valid)
        uevt->show = 1;
}

static int uevent_next_event(uevent_cb cb)
{
    struct pollfd fds;
    char buffer[UEVENT_MSG_LEN + 2];
    int count, nr;
    struct uevent uevt;

    while (1) {
        fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;

        nr = poll(&fds, 1, -1);

        if(nr > 0 && ((fds.revents & POLLIN) == POLLIN)) {
            memset(buffer, 0, UEVENT_MSG_LEN+2);
            count = recv(fd, buffer, UEVENT_MSG_LEN+2, 0);
            if (count > 0) {
               parse_event(buffer, &uevt);
               (*cb)(&uevt);
            }
        }
    }
    return 0;
}

/* uevent callback function
 */
static void on_uevent(struct uevent *uevt)
{
    uint32_t idx = 0;
    if (uevt->show) {
        UEVT_LOGD("\n==== uevent received done, envc: %2d ====", uevt->envc);
        while (uevt->envv[idx]) {
            UEVT_LOGD("%s", uevt->envv[idx]);
            idx++;
        }
    }
}

static void *uevent_monitor_thread(void *param)
{
    UEVT_LOGD("uevent monitor thread run");

    uevent_init();
    uevent_next_event(on_uevent);

    return param;
}

int show_help(const char *name)
{
    int i;
    UEVT_LOGD("Usage: %s [FILTER OPTION...]\n", name);

    for (i = 0; i < ARRAY_SIZE(option_list); i++)
        UEVT_LOGD("\t-%c, --%-10s %s",
                    option_list[i].long_options.val,
                    option_list[i].long_options.name,
                    option_list[i].help);

    UEVT_LOGD("\n");
    UEVT_LOGD("If no filter defined, %s will show all uevent.", name);
    return 0;
}

int main(int argc __unused, char **argv __unused)
{
    int ret, opt, i;
    pthread_t pid;
    pthread_attr_t attr;

    struct option long_options[ARRAY_SIZE(option_list)];
    for (i = 0; i < ARRAY_SIZE(option_list); i++) {
        long_options[i] = option_list[i].long_options;
    }

    memset(&filter, 0, sizeof(filter));

    while ((opt = getopt_long(argc, argv, "a:b:d:i:p:s:h", long_options, NULL)) != -1) {
        switch(opt) {
            case 'a':
                filter.action = optarg;
                filter.valid++;
                break;
            case 'b':
                filter.bus = optarg;
                filter.valid++;
                break;
            case 'd':
                filter.driver = optarg;
                filter.valid++;
                break;
            case 'i':
                filter.id = optarg;
                filter.valid++;
                break;
            case 'p':
                filter.devpath = optarg;
                filter.valid++;
                break;
            case 's':
                filter.subsystem = optarg;
                filter.valid++;
                break;
            default:
                show_help(argv[0]);
                return 0;
        }
    }

    ret = pthread_attr_init (&attr);
    if (ret != 0) {
        UEVT_LOGE("err: pthread_attr_init failed err=%s", strerror(ret));
        return ret;
    }

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
        UEVT_LOGE("err: pthread_attr_setdetachstate failed err=%s", strerror(ret));
        return ret;
    }

    ret = pthread_create(&pid, &attr, uevent_monitor_thread, NULL);
    if (ret) {
        UEVT_LOGE("err: pthread_create failed, ret=%d\n", ret);
        return ret;
    }

    while (true) {
        sleep(UINT32_MAX);
    }
    return 0;
}
