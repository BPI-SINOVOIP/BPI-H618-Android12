#define LOG_TAG "insmodDevices"
#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <linux/input.h>
#include <cutils/atomic.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <cutils/misc.h>
#include <cutils/properties.h>
#include "sensors.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

extern  "C" int init_module(void *, unsigned long, const char *);
extern  "C" int delete_module(const char *, unsigned int);

//#define DEBUG 1

#define INSMOD_PATH                     ("/vendor/lib/modules/")
#define SENSOR_MODULES_CFG              ("/vendor/etc/sensor_modules.cfg")
#define LINE_LENGTH  (128)

#define MODULE_HANDLE                   ("handle")
#define MODULE_PATH                     ("path")
#define MODULE_PREINSMOD                ("preinsmod")

struct sensor_module_type_t {
    int handle;
    char *module_path;
};

struct sensor_module_type_t insmod_list[SUPPORT_SENSORS_NUMBER];

static char name[LINE_LENGTH] = {'\0'};

/*
static int insmod(const char *filename, const char *args)
{

        void *module;
        unsigned int size;
        int ret;

        module = load_file(filename, &size);
        if (!module)
                return -1;

        ret = init_module(module, size, args);

        free(module);

        return ret;
}


static int rmmod(const char *modname)
{
        int ret = -1;
        int maxtry = 10;

        while (maxtry-- > 0) {
                ret = delete_module(modname, O_NONBLOCK | O_EXCL);
                if (ret < 0 && errno == EAGAIN)
                        usleep(500000);
                else
                        break;
        }

        if (ret != 0)
        ALOGD("Unable to unload driver module \"%s\": %s\n",
                modname, strerror(errno));
        return ret;
}

static char * get_module_name(char * buf)
{

        char ch = '\"';
        char * position1 ;
        char * position2 ;
        int s1 = 0,s2 = 0,k = 0;

#ifdef  DEBUG
        ALOGD("buf:%s",buf);
#endif
        if(!strlen(buf)){
                ALOGD("the buf is null !");
                return NULL;
        }
        memset(&name,0,sizeof(name));
        position1 = strchr(buf,ch);
        position2 = strrchr(buf,ch);
        s1 = position1 - buf + 1;
        s2 = position2 - buf;
#ifdef  DEBUG
        ALOGD("position1:%s,position2:%s,s1:%d,s2:%d",position1,position2,s1,s2);
#endif
        if((position1 == NULL) ||(position2 == NULL) || (s1 == s2)){
                return NULL;
        }
        while(s1 != s2)
        {
                name[k++] = buf[s1++];
        }
        name[k] = '\0';

#ifdef  DEBUG
        ALOGD("name : %s\n",name);
#endif

        return name;
}

static int insmod_modules(char * buf)
{
        char * module_name;
        char insmod_name[128];
        char ko[] = ".ko";
        int  i = 0;
        memset(&insmod_name,0,sizeof(insmod_name));

        module_name = get_module_name(buf);

#ifdef DEBUG
       ALOGD("begin!make sure to get the module_name!!!!");
#endif

        while ((module_name == NULL) && (i < 3)) {
            usleep(500000);
            module_name = get_module_name(buf);
            i++;
            };

#ifdef DEBUG
        ALOGD("module_name:%s.If it is NULL,system will not insmod that module's driver\n",module_name);
#endif
        if(module_name != NULL){
                sprintf(insmod_name,"%s%s%s",INSMOD_PATH,module_name,ko);

#ifdef DEBUG
                ALOGD("start to insmod %s\n",insmod_name);
#endif
                if (insmod(insmod_name, "") < 0) {
                        ALOGD(" %s insmod failed!\n",insmod_name);
                        rmmod(module_name);//it may be load driver already,try remove it and insmod again.
                        if (insmod(insmod_name, "") < 0){
                                ALOGD("%s,Once again fail to load!",insmod_name);
                                return 0;
                        }
                }
        }
        return 1;
}
*/
static int is_alpha(char chr)
{
        return ((chr >= 'a') && (chr <= 'z') ) ? 0 : 1;
}

static int get_handle(char *buf) {
    char *val;
    val = strtok(buf, "=");
    if (val != NULL) {
        val = strtok(NULL, " \n\r\t");
        if (!strncmp(val, "ID_A", 4))
            return ID_A;
        if (!strncmp(val, "ID_M", 4))
            return ID_M;
        if (!strncmp(val, "ID_GY", 5))
            return ID_GY;
        if (!strncmp(val, "ID_L", 4))
            return ID_L;
        if (!strncmp(val, "ID_PX", 5))
            return ID_PX;
        if (!strncmp(val, "ID_O", 4))
            return ID_O;
        if (!strncmp(val, "ID_T", 4))
            return ID_T;
        if (!strncmp(val, "ID_P", 4))
            return ID_P;
    }
    return -1;
}

static char* get_modulepath(char *buf) {
    char *path = NULL;
    char *val;

    path = (char *)malloc(LINE_LENGTH);
    if (path == NULL)
        return NULL;
    val = strtok(buf, "=");
    if (val != NULL) {
        val = strtok(NULL, " \n\r\t");
        if (val[0] == '/')
            sprintf(path, "%s", val);
        else
            sprintf(path, "%s%s", INSMOD_PATH, val);
        return path;
    }
    free(path);
    return NULL;
}

int insmodDevice(void)
{
    FILE *fp;
    int fd;
    int ret;
    char buf[LINE_LENGTH] = {0};
    int handle = -1;

    memset(&buf,0,sizeof(buf));
    if ((fp = fopen(SENSOR_MODULES_CFG, "r")) == NULL) {
        ALOGD("can't not open file!\n");
        return 0;
    }

    while (fgets(buf, LINE_LENGTH , fp)) {
#ifdef DEBUG
        ALOGD("buf:%s\n",buf);
#endif
        if  (!is_alpha(buf[0])) {
            if (!strncmp(buf, MODULE_HANDLE, 6))
                handle = get_handle(buf);
            else if (!strncmp(buf, MODULE_PATH, 4)) {
                if (handle > -1 && handle < SUPPORT_SENSORS_NUMBER) {
                    insmod_list[handle].handle = handle;
                    insmod_list[handle].module_path = get_modulepath(buf);
                    fd = TEMP_FAILURE_RETRY(open(insmod_list[handle].module_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
                    if (fd < 0) {
                        ALOGD("open %s failed:%s", insmod_list[handle].module_path, strerror(errno));
                        continue;
                    }
                    ret = syscall(__NR_finit_module, fd, "", 0);
                    if (ret < 0) {
                        ALOGD("finit_mdoule for %s failed:%s, fd = %d", insmod_list[handle].module_path,
                                strerror(errno), fd );
                    }
                    close(fd);
                    handle = -1;
                }
            } else if (!strncmp(buf, MODULE_PREINSMOD, 9)) {
                char* module = get_modulepath(buf);
                int fd = TEMP_FAILURE_RETRY(open(module, O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
                if (fd < 0) {
                    ALOGD("open %s failed:%s", module, strerror(errno));
                    continue;
                }
                ret = syscall(__NR_finit_module, fd, "", 0);
                if (ret < 0) {
                    ALOGD("finit_mdoule for %s failed:%s, fd = %d", module, strerror(errno), fd );
                }
                close(fd);
            }
        }
        memset(&buf,0,sizeof(buf));
    }

    fclose(fp);
    return 0;
}
