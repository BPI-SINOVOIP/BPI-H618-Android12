#ifndef __SUN8IW5P1_A33_H__
#define __SUN8IW5P1_A33_H__
/* cpu spec files defined */
#define CPU0LOCK    "/sys/devices/system/cpu/cpu0/cpufreq/boot_lock"
#define ROOMAGE     "/sys/devices/soc/cpu_budget_cool.7/roomage"
#define CPUFREQ     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define CPUONLINE   "/sys/devices/system/cpu/online"
#define CPUHOT      "/sys/kernel/autohotplug/enable"
#define CPU0GOV     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
/* gpu spec files defined */
#define GPUFREQ     "/sys/devices/1c40000.gpu/dvfs/android"
/* ddr spec files defined */
#define DRAMFREQ    "/sys/devices/platform/sunxi-ddrfreq/devfreq/sunxi-ddrfreq/cur_freq"
#define DRAMSCEN    "/sys/class/devfreq/sunxi-ddrfreq/dsm/scene"
/* task spec files defined */
#define TASKS       "/dev/cpuctl/tasks"

#define GPUCOMMAND "/sys/devices/gpu_mali400_0/scenectrl/command" //mode of  GPU scense control:0,1
#define GPUSTATUS  "/sys/devices/gpu_mali400_0/scenectrl/status" //switch of  GPU scense control:0,1


/*  value define */
#define ROOMAGE_NORMAL     "0 0 0 0 1200000 4 0 0 0"
#define ROOMAGE_BENCHMARK  "1200000 4 0 0 1200000 4 0 0"
#define ROOMAGE_SCREEN_OFF "0 0 0 0 1200000 4 0 0 0"
#define ROOMAGE_VR           "1200000 4 0 0 1200000 4 0 0"
#define ROOMAGE_SUSTAINED    "816000 4 0 0 816000 4 0 0"
#define ROOMAGE_VR_SUSTAINED "1008000 4 0 0 1008000 4 0 0"
#define ROOMAGE_LAUNCH       "1200000 4 0 0 1200000 4 0 0"
#define ROOMAGE_HOME         "0 0 0 0 1200000 4 0 0 0"
#define ROOMAGE_MUSIC        "0 0 0 0 1200000 4 0 0 0"

/* dram scene value defined */
#define DRAM_NORMAL         "0"
#define DRAM_HOME           "1"
#define DRAM_LOCALVIDEO     "2"
#define DRAM_BGMUSIC        "3"
#define DRAM_4KLOCALVIDEO   "4"
/* gpu scene value defined */
#define GPU_NORMAL          "4\n"
#define GPU_HOME            "4\n"
#define GPU_LOCALVIDEO      "4\n"
#define GPU_BGMUSIC         "4\n"
#define GPU_4KLOCALVIDEO    "4\n"
#define GPU_PERF            "8\n"

/*thermal value*/
#define THERMAL_EMUL_TEMP   "/sys/class/thermal/thermal_zone0/emul_temp"
#define THERMAL_CLOSE_EMUL_TEMP  "30"



#endif
