#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <cutils/properties.h>
#include <hardware/power.h>
#include "power-common.h"

static Power_Perfromance_State g_cur_power_state_test = POWER_STATE_NORMAL;

void sysfs_write(const char *path, char *s)
{
    char buf[64];
    int len;
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        strerror_r(errno, buf, sizeof(buf));
        //ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }
    len = write(fd, s, strlen(s));
    if (len < 0)
    {
        strerror_r(errno, buf, sizeof(buf));
        //ALOGE("Error writing to %s: %s\n", path, buf);
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

Power_Perfromance_State get_cur_power_state() {
	return g_cur_power_state_test;
}

void set_power_state(Power_Perfromance_State state) {
	g_cur_power_state_test = state;
}


void  command_power_hint(int hint, void *data)
{

	int parmdata = 0; //default value=0 if data is null pointer
	if(data != NULL) {
		parmdata = *((int* )data);
	}

	power_hint_platform(hint,parmdata);

 }


int main(int argc, char **argv)
{
    char propval[100]={0};
    if(argc<2 )
    {
        printf("missing arguments\n");
        return 0;
    }
    if(!strcmp(argv[1],"resume"))
    {
        if(argc!= 2)
        {
            printf("Usage:resume");
            return -1;
        }
        printf("quit debug mode!\n");
        property_set("persist.vendor.p_debug","false");
        property_set("persist.vendor.p_bootcomplete","true");
        property_set("persist.vendor.p_benchmark","true");
        property_set("persist.vendor.p_normal","true");
        property_set("persist.vendor.p_music","true");
        return 0;
    }

    if(!strcmp(argv[1],"enter"))
    {
        if(!strcmp("benchmark", argv[2]))
        {
            if(argc!=4)
            {
                printf("Usage:enter benchmark [pid0]\n");
                return -1;
            }
            property_get("persist.vendor.p_benchmark", propval,"false");
            if(!strcmp(propval,"false"))
            {
                printf("benchmark mode has been disabled!\n");
                return -1;
            }
            property_set("persist.vendor.p_debug","true");
            command_power_hint(POWER_HINT_BENCHMARK, argv[3]);
            printf("benchmark succeed!\n");
        }

        else if(!strcmp("normal",argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enter normal\n");
                return -1;
            }
            property_get("persist.vendor.p_normal",propval,"false");
            if(!strcmp(propval,"false"))
            {
                printf("normal mode has been disabled!\n");
                return -1;
            }
            property_set("persist.vendor.p_debug","true");
            command_power_hint(POWER_HINT_NORMAL, "1");
            printf("normal succeed!\n");
        }
        else if(!strcmp("music",argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enter music\n");
                return -1;
            }
            property_get("persist.vendor.p_music" , propval,"false");
            if(!strcmp(propval,"false"))
            {
                printf("music mode has been disabled!\n");
                return -1;
            }
            property_set("persist.vendor.p_debug","true");
            command_power_hint(POWER_HINT_BG_MUSIC, "2");
            printf("music succeed!\n");
        }

        else if(!strcmp("bootcomplete",argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enter bootcomplete\n");
                return -1;
            }
            property_get("persist.vendor.p_bootcomplete",propval,"false");
            if(!strcmp(propval,"false"))
            {
                printf("benchmark mode has been disabled!\n");
                return -1;
            }
            property_set("persist.vendor.p_debug","true");
            command_power_hint(POWER_HINT_BOOTCOMPLETE,"false");
            printf("bootcomplete succeed!\n");
        }
        else
        {
            printf("un-defined command:%s!\n", argv[1]);
            return -1;
        }
    }

    else if(!strcmp(argv[1],"enable"))
    {
        if(!strcmp("benchmark", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enable benchmark");
                return -1;
            }
            property_set("persist.vendor.p_benchmark","true");
            printf("enable benchmark succeed!\n");
        }
        else if(!strcmp("normal", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enable normal\n");
                return -1;
            }
            property_set("persist.vendor.p_normal","true");
            printf("enable normal  succeed!\n");
        }

        else if(!strcmp("music", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enable music\n");
                return -1;
            }
            property_set("persist.vendor.p_music","true");
            printf("enable music succeed!\n");
        }

        else  if(!strcmp("bootcomplete", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:enable bootcomplete\n");
                return -1;
            }
            property_set("persist.vendor.p_bootcomplete","true");
            printf("enable bootcomplete succeed!\n");
        }
        else
        {
            printf("un-defined command:%s!\n",argv[1] );
            return -1;
        }
    }


    else if(!strcmp(argv[1],"disable"))
    {
        if(!strcmp("benchmark", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:disable benchmark");
                return -1;
            }
            property_set("persist.vendor.p_benchmark","false");
            printf("disable benchmark succeed!\n");
        }
        else if(!strcmp("normal", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:disable normal\n");
                return -1;
            }
            property_set("persist.vendor.p_normal","false");
            printf("disable normal succeed!\n");
        }
        else if(!strcmp("music", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:disable music\n");
                return -1;
            }
            property_set("persist.vendor.p_music","false");
            printf("disable music succeed!\n");
        }

        else  if(!strcmp("bootcomplete", argv[2]))
        {
            if(argc!=3)
            {
                printf("Usage:disable bootcomplete\n");
                return -1;
            }
            property_set("persist.vendor.p_bootcomplete","false");
            printf("disable bootcomplete succeed!\n");
        }
        else
        {
            printf("un-defined command:%s\n", argv[1]);
            return -1;
        }
    }

    else
    {
        printf("un-defined command:%s\n",argv[1]);
        return -1;
    }
    return 0;
}

