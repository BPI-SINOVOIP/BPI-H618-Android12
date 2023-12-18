#ifndef __SUN8IW6P1_T8_H__
#define __SUN8IW6P1_T8_H__
/* cpu spec files defined */
#define ROOMAGE     "/sys/devices/platform/soc/cpu_budget_cooling/roomage"
#define CPUFREQ     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define CPUONLINE   "/sys/devices/system/cpu/online"
#define CPUHOT      "/sys/kernel/autohotplug/enable"
#define CPU0GOV     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
/* gpu spec files defined */
/* ddr spec files defined */
/* task spec files defined */
#define TASKS       "/dev/cpuctl/tasks"
/* touch screen runtime suspend */
#define TP_SUSPEND  "/sys/devices/platform/soc/twi0/i2c-0/0-0040/input/input4/runtime_suspend"

/*  value define */
#define ROOMAGE_NORMAL       "0 0 0 0 1800000 4 1800000 4"
#define ROOMAGE_BENCHMARK    "1800000 4 1800000 4 1800000 4 1800000 4"
#define ROOMAGE_LAUNCH       "1800000 4 1800000 4 1800000 4 1800000 4"
#define ROOMAGE_HOME         "0 0 0 0 1200000 4 0 0"
#define ROOMAGE_SCREEN_OFF   "0 0 0 0 1200000 4 0 0"
#define ROOMAGE_MUSIC        "0 0 0 0 1200000 4 0 0"

/* dram scene value defined */
/* gpu scene value defined */

/*thermal value*/
#define THERMAL_EMUL_TEMP   "/sys/class/thermal/thermal_zone0/emul_temp"
#define THERMAL_CLOSE_EMUL_TEMP  "50"
#define THERMAL_OPEN_EMUL_TEMP   "0"
#endif
