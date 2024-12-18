/*
 * Copyright (C) 2018 The Allwinnertech
 *
 *
 * Author:jiangbin@allwinnertech.com
 * Time:2090715
 *
 * description:
 *     Target TARGET_BOARD_PLATFORM: default(frome A50)
 *     power_hint-policy
 */

#include <string.h>
#include <stdio.h>

#define  LOG_TAG "AW_PowerHAL_Platform"
#include <hardware/hardware.h>
#include <hardware/power.h>
#include <utils/Log.h>
#include "../power-common.h"

#ifndef CPUMAXCORENUM
#define CPUMAXCORENUM    "4"
#endif

#ifndef CPUMAXFREQ
#define CPUMAXFREQ       "1800000"
#endif

#ifndef CPUMINFREQ
#define CPUMINFREQ       "480000"
#endif

#ifndef CPULOWPOWERFREQ
#define CPULOWPOWERFREQ  "1200000"
#endif

#ifndef CPUNORMALFREQ
#define CPUNORMALFREQ  cpu0_max_freq
#endif

#ifndef CPU0OPP
#define CPU0OPP     "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"
#endif

void power_set_interactive_platform(int enable)
{
    (void)enable;
}


/*
 *return: false -check fail
 *        true  -check success
*/
static bool check_policy_platform(unsigned int hint, int parmdata)
{
    (void)hint;
    (void)parmdata;
    return true;
}

static char cpu0_max_freq[16] = {0};

bool power_hint_platform(unsigned int hint, int parmdata)
{
    Power_Perfromance_State current_state = get_cur_power_state();

    if (!check_policy_platform(hint, parmdata)) {
        ALOGI("==default check policy fail,do nothing==");
        return false;
    }

    if (!cpu0_max_freq[0])
        cpufreq_max_get(CPU0OPP, cpu0_max_freq);

    bool hitcase = true;
    switch (hint) {
        case POWER_HINT_LAUNCH:
            sysfs_write(THERMAL_EMUL_TEMP,THERMAL_OPEN_EMUL_TEMP);
            if ((parmdata == 1) && (current_state != POWER_STATE_LAUNCH)) {
                set_power_state(POWER_STATE_LAUNCH);
                //sysfs_write(ROOMAGE,ROOMAGE_LAUNCH);
                sysfs_write(CPULOCK,CPUMAXCORENUM);
                sysfs_write(CPUMINFREQCONTROL, cpu0_max_freq);
                sysfs_write(CPUMAXFREQCONTROL, cpu0_max_freq);
                ALOGI("==LAUNCH_MODE==");

            } else if (current_state == POWER_STATE_LAUNCH) {
                set_power_state(POWER_STATE_NORMAL);
                //sysfs_write(ROOMAGE,ROOMAGE_NORMAL);
                sysfs_write(CPULOCK,"0");
                sysfs_write(CPUMINFREQCONTROL,CPUMINFREQ);
                sysfs_write(CPUMAXFREQCONTROL,CPUNORMALFREQ);
                ALOGI("==LAUNCH_NORMAL==");
            } else {
                ALOGE("==HINT_LAUNCH_ERROR==");
            }
            break;

        case POWER_HINT_HOME:
            ALOGI("==HOME MODE==");
            sysfs_write(THERMAL_EMUL_TEMP,THERMAL_OPEN_EMUL_TEMP);
            sysfs_write(CPULOCK,"0");
            sysfs_write(CPUMAXFREQCONTROL,CPULOWPOWERFREQ);
            set_power_state(POWER_STATE_HOME);
            break;

        case POWER_HINT_BOOTCOMPLETE:
            ALOGI("==BOOTCOMPLETE MODE==");
            sysfs_write(CPU0GOV,INTERACTIVE_GOVERNOR);
            sysfs_write(CPUHOT,"1");
            break;

        case POWER_HINT_BENCHMARK:
            if (current_state == POWER_STATE_BENCHMARK) {
                ALOGD("HINT_BENCHMARK:same state");
                return hitcase;
            }

            if (parmdata == 0) {
                ALOGE("HINT_BENCHMARK:data==NULL");
                return hitcase;
            } else {
                 ALOGD("HINT_BENCHMARK:data =%d",parmdata);
            }

            sysfs_write(THERMAL_EMUL_TEMP,THERMAL_CLOSE_EMUL_TEMP);
            //sysfs_write(ROOMAGE,ROOMAGE_BENCHMARK);
            sysfs_write(CPULOCK,CPUMAXCORENUM);
            sysfs_write(CPUMINFREQCONTROL, cpu0_max_freq);
            sysfs_write(CPUMAXFREQCONTROL, cpu0_max_freq);

            set_power_state(POWER_STATE_BENCHMARK);
            ALOGI("==BENCHMARK MODE==");
            break;

        case POWER_HINT_NORMAL:
            ALOGI("==NORMAL MODE==");
            sysfs_write(CPU0GOV,INTERACTIVE_GOVERNOR);
            sysfs_write(THERMAL_EMUL_TEMP,THERMAL_OPEN_EMUL_TEMP);
            //sysfs_write(ROOMAGE,ROOMAGE_NORMAL);
            sysfs_write(CPULOCK,"0");
            sysfs_write(CPUMINFREQCONTROL,CPUMINFREQ);
            sysfs_write(CPUMAXFREQCONTROL,CPUNORMALFREQ);
            set_power_state(POWER_STATE_NORMAL);
            break;

        case POWER_HINT_BG_MUSIC:
            ALOGI("==BG_MUSIC MODE==");
            sysfs_write(THERMAL_EMUL_TEMP,THERMAL_OPEN_EMUL_TEMP);
            //sysfs_write(ROOMAGE, ROOMAGE_MUSIC);
            sysfs_write(CPULOCK,"0");
            sysfs_write(CPUMAXFREQCONTROL,CPULOWPOWERFREQ);
            set_power_state(POWER_STATE_MUSIC);
            break;

        case POWER_HINT_DISABLE_TOUCH:
            ALOGI("==DISABLE TOUCH==");
            char tp_state[2]= {0};
            sprintf(tp_state,"%d", parmdata);
            sysfs_write(TP_SUSPEND, tp_state);
            break;

        default:
            hitcase = false;
            break;

    }

    return hitcase;
}

