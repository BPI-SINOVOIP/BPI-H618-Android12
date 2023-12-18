#ifndef ___SUN50IW9P1_H616H__
#define ___SUN50IW9P1_H616H__
/* cpu spec files defined */
#define CPU0LOCK    "/sys/devices/system/cpu/cpu0/cpufreq/boot_lock"
#define ROOMAGE     "/sys/devices/platform/soc/cpu_budget_cooling/roomage"
#define CPUFREQ     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define CPUONLINE   "/sys/devices/system/cpu/online"
#define CPUHOT      "/sys/kernel/autohotplug/enable"
#define CPUBOOSTALL     "/sys/kernel/autohotplug/boost_all"
#define CPU0GOV     "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPU0OPP     "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"

/* gpu spec files defined */
#define GPUFREQ     "/sys/devices/1c40000.gpu/dvfs/android"
/* ddr spec files defined */
#define DRAMFREQ    "/sys/class/devfreq/dramfreq/cur_freq"
#define DRAMPAUSE    "/sys/class/devfreq/dramfreq/adaptive/pause"
/* task spec files defined */
#define TASKS       "/dev/cpuctl/tasks"
/* touch screen runtime suspend */
#define TP_SUSPEND  "/sys/devices/platform/soc/twi0/i2c-0/0-0040/input/input6/runtime_suspend"

#define GPUCOMMAND "/sys/devices/platform/gpu/scenectrl/command" //mode of  GPU scense control:0,1
#define GPUSTATUS  "/sys/devices/platform/gpu/scenectrl/status" //switch of  GPU scense control:0,1

/*  value define */
#define ROOMAGE_PERF         " 1416000 4 0 0  1416000 4 0 0"
#define ROOMAGE_NORMAL       "0 0 0 0  1416000 4 0 0 0"
#define ROOMAGE_VIDEO        "0 0 0 0 1  1416000 4 0 0"
#define ROOMAGE_VR           "1416000 4 0 0 1416000 4 0 0"
#define ROOMAGE_SUSTAINED    "816000 4 0 0 816000 4 0 0"
#define ROOMAGE_VR_SUSTAINED "1200000 4 0 0 1200000 4 0 0"

#define ROOMAGE_LAUNCH       "1800000 4 0 0 1800000 4 0 0"
#define ROOMAGE_BENCHMARK    "1800000 4 0 0 1800000 4 0 0"
#define ROOMAGE_SCREEN_OFF   "480000 4 0 0 1200000 4 0 0"
#define ROOMAGE_HOME         "0 0 0 0 1200000 4 0 0 0"
#define ROOMAGE_MUSIC        "0 0 0 0 1200000 4 0 0 0"


/*new*/
#define CPUMAXFREQ   "1416000 "
#define CPUMINFREQ   "816000"
#define CPULOWPOWERFREQ   "1200000"
#define CPUMAXCORENUM   "4"


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
#define THERMAL_CLOSE_EMUL_TEMP  "50"
#define THERMAL_OPEN_EMUL_TEMP   "0"
#endif

