#ifndef __SUN8IW6P1_A80_H__
#define __SUN8IW6P1_A80_H__

/* cpu spec files defined */
#define CPU0LOCK    "/sys/devices/system/cpu/cpu0/cpufreq/boot_lock"
#define CPU4LOCK    "/sys/devices/system/cpu/cpu4/cpufreq/boot_lock"
#define ROOMAGE     "/sys/devices/platform/sunxi-budget-cooling/roomage"
#define CPUFREQ     "/sys/devices/system/cpu/cpu0/cpufreq/scale_cur_freq"
#define CPUONLINE   "/sys/devices/system/cpu/online"
#define CPUHOT      "/sys/kernel/autohotplug/enable"
#define CPU0GOV     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPU4GOV     "/sys/devices/system/cpu/cpu4/cpufreq/scaling_governor"
/* gpu spec files defined */
#define GPUFREQ     "/sys/devices/platform/pvrsrvkm/dvfs/android"
/* ddr spec files defined */
#define DRAMFREQ    "/sys/devices/platform/sunxi-ddrfreq/devfreq/sunxi-ddrfreq/cur_freq"
#define DRAMSCEN    "/sys/class/devfreq/sunxi-ddrfreq/dsm/scene"
/* task spec files defined */
#define TASKS       "/dev/cpuctl/tasks"

/* value define */
#define ROOMAGE_NORMAL     "0       0       0 0 1200000 4 1608000 4 0"
#define ROOMAGE_BENCHMAKR  "1200000 4 1296000 4 1200000 4 1800000 4"
#define ROOMAGE_LAUNCH  "1200000 4 1296000 4 1200000 4 1800000 4"

#define ROOMAGE_HOME         "0 0 0 0 1200000 4 0 0 0"
#define ROOMAGE_MUSIC        "0 0 0 0 1200000 4 0 0 0"
/* dram scene value defined */
#define DRAM_NORMAL         "0"
#define DRAM_HOME           "1"
#define DRAM_LOCALVIDEO     "2"
#define DRAM_BGMUSIC        "3"
#define DRAM_4KLOCALVIDEO   "4"
/* gpu scene value defined */
#define GPU_NORMAL          "384\n"
#define GPU_HOME            "384\n"
#define GPU_LOCALVIDEO      "384\n"
#define GPU_BGMUSIC         "384\n"
#define GPU_4KLOCALVIDEO    "384\n"
#define GPU_PERF            "528\n"
#endif
