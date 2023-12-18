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

#include <cutils/klog.h>

#include "aw_healthd.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <android-base/logging.h>
#include <fcntl.h>
#define LOGE(x...) KLOG_ERROR("charger", x);
#define LOGW(x...) KLOG_WARNING("charger", x);
#define LOGV(x...) KLOG_DEBUG("charger", x);
#include <suspend/autosuspend.h>
#include <sys/reboot.h>
#include <hardware/sunxi_display2.h>
#include <pthread.h>
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
#define DISP_ID 0
static bool suspend_flag = false;
int backlight_on(int ret)
{
    unsigned long  args[3];
    int err = 0;
    int disp_fd;
    static int value;
    static int init_ok;
    if((disp_fd = open("/dev/disp", O_RDONLY)) < 0)
    {
        LOGW("backlight_on open disp_fd failed!\n");
        return -1;
    }

    pthread_mutex_lock(&g_lock);
    args[0] = DISP_ID;
    if (ret == 0) {
        args[1] = 0;
    } else {
        args[1] = 100;
    }
    if ((!init_ok) || (value != ret)){
        init_ok = 1;
        value = ret;
        LOGW("backlight_on set lcd%lu lcd_bl:%lu\n",args[0],args[1]);
        if(ioctl(disp_fd,DISP_GET_OUTPUT_TYPE,args) == DISP_OUTPUT_TYPE_LCD)
        {
            err = ioctl(disp_fd,DISP_LCD_SET_BRIGHTNESS,args);
        }
    }
    pthread_mutex_unlock(&g_lock);

    if(disp_fd)
        close(disp_fd);
    return err;
}


int request_suspend(bool enable)
{

    suspend_flag = enable;
    if (enable) {
        backlight_on(0);
        return autosuspend_enable();
    }
    else {
        backlight_on(1);
        return autosuspend_disable();
    }
}

#define WAKEALARM_PATH        "/sys/class/rtc/rtc0/wakealarm"
#define ALARM_IN_BOOTING_PATH        "/sys/module/rtc_sunxi/parameters/alarm_in_booting"
static int wakealarm_fd;
static int epollfd;
static pthread_t tid_alarm;
#define EPOLL_LISTEN_CNT 1
static long get_wakealarm_sec(void)
{
    int fd = 0, ret = 0;
    unsigned long wakealarm_time = 0;
    char buf[32] = { 0 };

    fd = open(WAKEALARM_PATH, O_RDWR);
    if (fd < 0) {
        LOGE("open %s failed, return=%d\n", WAKEALARM_PATH, fd);
        return fd;
    }

    ret = read(fd, buf, sizeof(buf));
    if (ret > 0) {
        wakealarm_time = strtoul(buf, NULL, 0);
        // Clean initial wakealarm.
        // We will set wakealarm again use timerfd_settime.
        // Initial wakealarm's time unit is second, have conflict with
        // nanosecond alarmtimer in alarmtimer_suspend().
        snprintf(buf, sizeof(buf), "0");
        write(fd, buf, strlen(buf) + 1);

        close(fd);
        return wakealarm_time;
    }

    close(fd);
    return ret;
}

static long is_alarm_in_booting(void)
{
    int fd = 0, ret = 0;
    unsigned long alarm_in_booting = 0;
    char buf[32] = { 0 };

    fd = open(ALARM_IN_BOOTING_PATH, O_RDONLY);
    if (fd < 0) {
        LOGE("open %s failed, return=%d\n", ALARM_IN_BOOTING_PATH, fd);
        return fd;
    }

    ret = read(fd, buf, sizeof(buf));
    if (ret > 0) {
        alarm_in_booting = strtoul(buf, NULL, 0);
    }
    LOG(WARNING) <<  __func__ <<" "<< __LINE__<<"alarm_in_booting： "<< alarm_in_booting << "\n";
    close(fd);
    return alarm_in_booting;
}


void *alarm_thread_handler(void *arg)
{
    (void)arg;
    struct epoll_event events[EPOLL_LISTEN_CNT];
    struct timeval now_tv = { 0, 0 };
    while (true) {
        int nevents = epoll_wait(epollfd, events,  EPOLL_LISTEN_CNT, -1);
        if (nevents < 0) {
            LOG(WARNING) <<  __func__ <<" ++++"<< __LINE__ << " event:"<< nevents<< "errno: "<< errno << "\n";
            continue;
        }
        unsigned long long wakeups;

        if (read(wakealarm_fd, &wakeups, sizeof(wakeups)) == -1) {
            LOGE("wakealarm_event: read wakealarm fd failed\n");
            continue;
        }
        gettimeofday(&now_tv, NULL);
        LOG(WARNING) <<  __func__ <<" " << "rebooting" << "now" << now_tv.tv_sec <<"\n";
        request_suspend(false);
        reboot(RB_AUTOBOOT);
    }

    return NULL;
}


static void init_shutdown_alarm(void)
{
    long alarm_secs, alarm_in_booting = 0;
    struct timeval now_tv = { 0, 0 };
    struct timespec ts;
    struct epoll_event ev;
    alarm_secs = get_wakealarm_sec();
    // have alarm irq in booting ?
    alarm_in_booting = is_alarm_in_booting();
    gettimeofday(&now_tv, NULL);

    LOG(WARNING) << "alarm_in_booting: "<< alarm_in_booting << "alarm_secs " << alarm_secs << "now" << now_tv.tv_sec << "\n";
    // alarm interval time == 0 and have no alarm irq in booting
    if (alarm_secs <= 0 && (alarm_in_booting != 1))
        return;
    if (alarm_secs)
        ts.tv_sec = alarm_secs;
    else
        ts.tv_sec = (long)now_tv.tv_sec + 1;

    ts.tv_nsec = 0;

    struct itimerspec spec;
    memset(&spec, 0, sizeof(spec));
    memcpy(&spec.it_value, &ts, sizeof(spec.it_value));

      //timerfd_init
    wakealarm_fd = timerfd_create(CLOCK_REALTIME_ALARM, 0);
    if (wakealarm_fd <= 0) {
        LOGE("%s, %d, alarm_fd=%d and exit\n", __func__, __LINE__, wakealarm_fd);
        return ;
    }

    if (timerfd_settime(wakealarm_fd, TFD_TIMER_ABSTIME, &spec, NULL) == -1){
        LOGE("timerfd_settime failed Error[%d:%s]\n",errno,strerror(errno));
        close(wakealarm_fd);
        return ;
    };
      //epoll_init
    epollfd = epoll_create(EPOLL_LISTEN_CNT);
    if (epollfd > 0) {
        ev.events = EPOLLIN | EPOLLWAKEUP;
        //add wakealarm_fd to epollfd
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, wakealarm_fd, &ev) == -1) {
            LOGE("epoll_ctl failed; errno=%d\n", errno);
            return;
        }
    } else {
            LOGE("epoll_create failed; errno=%d\n", errno);
            return;
    }
    pthread_create(&tid_alarm, NULL, alarm_thread_handler, NULL);
    return;
}

void aw_healthd_init()
{
    init_shutdown_alarm();
}
