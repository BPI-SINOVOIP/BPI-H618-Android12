/*
 * Copyright (C) 2020 Allwinner Technology
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

#pragma once

// cpufreq
#define CPU0GOV "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
// PERFORMANCE: CPU will run at a fixed max frequency.
#define PERFORMANCE_GOVERNOR "performance"
// SCHEDUTIL: CPU will run at a dynamic frequency which will be decided by system load.
#define SCHEDUTIL_GOVERNOR "schedutil"
// CONSERVATIVE: CPU will try to run at a min frequency .
#define CONSERVATIVE_GOVERNOR "conservative"

// thermal
#define THERMAL_EMUL_TEMP "/sys/class/thermal/thermal_zone0/emul_temp"
// Set emulated temperature to 50 degree, so that thermal control take no effect.
#define THERMAL_CLOSE_EMUL_TEMP "50"
// Set emulated temperature to 0 degree and this will disable temperation emulation, so that thermal control will take effect.
#define THERMAL_OPEN_EMUL_TEMP "0"
// thermal-new
#define THERMAL_MODE "/sys/class/thermal/thermal_zone0/mode"
#define THERMAL_ENABLE "enabled"
#define THERMAL_DISABLE "disabled"

// TP-suspend
#define TP_SUSPEND_CONTROL "/sys/class/ctp/tp_idle"


//IO-Scheduler
#define IO_SCHEDULER "/sys/block/mmcblk0/queue/scheduler"
//mq-deadline
#define MQ_READ_EXPIRE "/sys/block/mmcblk0/queue/iosched/read_expire"
#define MQ_WRITE_EXPIRE "/sys/block/mmcblk0/queue/iosched/write_expire"
#define MQ_WRITES_STARVED "/sys/block/mmcblk0/queue/iosched/writes_starved"
#define MQ_FIFO_BATCH "/sys/block/mmcblk0/queue/iosched/fifo_batch"
