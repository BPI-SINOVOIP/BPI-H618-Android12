/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

#define  LOG_TAG "AW_PowerHAL"
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/power.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <unistd.h>
#include "power-common.h"



static pthread_mutex_t s_interaction_lock = PTHREAD_MUTEX_INITIALIZER;


static Power_Perfromance_State g_cur_power_state = POWER_STATE_NORMAL;
static Power_Perfromance_State g_pre_power_state = POWER_STATE_NORMAL;
static bool g_enable_power_control = true;
static bool g_boot_complete = false;

void sysfs_write(const char *path, char *s)
{
    char buf[64];
    int len;
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }
    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }
    close(fd);
}

void cpufreq_max_get(const char *path, char *s)
{
    char *max_freq, *p, buf[64], cpu_opp[128] = {0};
    int len, fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }
    len = read(fd, cpu_opp, sizeof(cpu_opp));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading from %s: %s\n", path, buf);
    }
    close(fd);

    for (p = cpu_opp, max_freq = p; ; p++) {
        if (strncmp(p, " ", 1))
            continue;
        if (!strncmp(p + 1, "\n", 1))
           break;

        max_freq = p + 1;
    }

    memcpy(s, max_freq, 8);
}

static void power_get_debug_property() {
   char propval[100]={0};
   property_get("persist.vendor.power.enable", propval,"true");
   if(!strcmp(propval,"false")) {
     ALOGI("==power_init disable scene control==\n");
     g_enable_power_control = false;
   }
}

static void power_init(struct power_module *module)
{
    //ALOGI("power_init: init success!!");

}

static void power_set_interactive(struct power_module *module, int on)
{
    power_get_debug_property();
    if(!g_enable_power_control) {
        ALOGI("==DISABLED DONOTHING== SET_INTER");
        return;
    }
    ALOGI("==SET_INTER %d",on);
    if(on) {
        power_hint_platform(POWER_HINT_NORMAL,on);
        set_power_state(POWER_STATE_NORMAL);
    } else {
        //reserve
        set_power_state(POWER_STATE_SCAREEN_OFF);
    }
    power_set_interactive_platform(on);

}

static void set_feature(struct power_module *module, feature_t feature, int state)
{
#if 0
    //ALOGI("set_feature: init success!!");
    struct power_module *aw = (struct power_module *) module;
    switch(feature) {
        default:
            ALOGW("Error setting the feature, it doesn't exist %d\n", feature);
        break;
    }
#endif
}

static ssize_t get_number_of_platform_modes(struct power_module *module) {
    //ALOGI("get_number_of_platform_modes: init success!!");
    return 0;
}

static int get_platform_low_power_stats(struct power_module *module,
    power_state_platform_sleep_state_t *list) {
    //ALOGI("get_platform_low_power_stats: init success!!");
    return 0;
}

static int get_voter_list(struct power_module *module, size_t *voter) {
    ALOGI("get_voter_list: init success!!");
    return 0;
}


Power_Perfromance_State get_pre_power_state() {
    return g_pre_power_state;
}

Power_Perfromance_State get_cur_power_state() {
    return g_cur_power_state;
}

void set_power_state(Power_Perfromance_State state) {
    g_pre_power_state = g_cur_power_state;
    g_cur_power_state = state;
}



/*
 *return: false -check fail
 *        true  -check success
*/
static bool check_policy_common(unsigned int hint, int parmdata) {

    //reserve

    //common - policy do nothing before boot_complete(because linux-start with performance-mode)
    if(!g_boot_complete && (hint != POWER_HINT_BOOTCOMPLETE)) {
        ALOGD("==DO NOTHING BEFORE BOOT_COMPLETE==");
        return false;
    }

    switch (hint) {
     case POWER_HINT_NORMAL:
        if((parmdata != 0) && (parmdata != get_cur_power_state())) {
         ALOGD("==NORMAL MODE parmdata invalid- do nothing== parm= %d ,current_state= %d",parmdata,get_cur_power_state());
         return false;
        }
         break;
     case POWER_HINT_BOOTCOMPLETE:
          g_boot_complete = true;
          break;
     default:
     break;
    }

    return true;

}



static void power_hint(struct power_module *module, power_hint_t hint,
                       void *data) {

        power_get_debug_property();

        if(!g_enable_power_control) {
            ALOGI("==DISABLED DONOTHING== HINT = 0x%x",hint);
            return;
        }
        pthread_mutex_lock(&s_interaction_lock);
        int parmdata = 0; //default value=0 if data is null pointer
        if(data != NULL) {
            parmdata = *((int* )data);
        }
        if(!check_policy_common(hint,parmdata)) {
            ALOGI("==check common policy fail,do nothing==");
            goto OUT;
        }


        if(power_hint_platform(hint,parmdata)) {
            goto OUT;
        }

        switch (hint) {
            case POWER_HINT_LOW_POWER:
            case POWER_HINT_INTERACTION:
                //ALOGI("reserve");
                pthread_mutex_unlock(&s_interaction_lock);
                return;
            default:
                ALOGD("need check-parm 0x%x",hint);
                break;
           }
    OUT:
        ALOGD("c-s = %d",get_cur_power_state());
        pthread_mutex_unlock(&s_interaction_lock);
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "AW Power HAL",
        .author = "The Android Open Source Project",
        .methods = &power_module_methods,
    },

    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
    .setFeature = set_feature,
    .get_number_of_platform_modes = get_number_of_platform_modes,
    .get_platform_low_power_stats = get_platform_low_power_stats,
    .get_voter_list = get_voter_list
};
