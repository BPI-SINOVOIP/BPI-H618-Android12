#define LOG_TAG "Multi_tpinsmod"
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <linux/input.h>
#include <android/log.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <cutils/misc.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

//#define DEBUG

extern  "C" int init_module(void *, unsigned long, const char *);
extern  "C" int delete_module(const char *, unsigned int);

#define TP_MODULES_CFG              ("/vendor/etc/tp_modules.cfg")
#define LINE_LENGTH  (128)

#define MODULE_NAME                     ("name")
#define MODULE_PATH                     ("path")

#define PROPERTY_NAME        ("persist.vendor.tp.name")
#define PROPERTY_PATH        ("persist.vendor.tp.path")

struct tp_module_config {
    char *name;
    char *module_path;
};

static int is_alpha(char chr)
{
    if ((chr >= 65 && chr <=90) || (chr >= 97 && chr <=122))
        return 0;

    return -1;
}

static char* get_module_name(char *buf)
{
    char *name = NULL;
    char *val;

    name = (char *)malloc(LINE_LENGTH);
    if (name == NULL)
        return NULL;
    val = strtok(buf, "=");
    if (val != NULL) {
        val = strtok(NULL, " \n\r\t");
        sprintf(name, "%s", val);
        return name;
    }
    free(name);
    return NULL;
}

static char* get_modulepath(char *buf)
{
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
        return path;
    }
    free(path);
    return NULL;
}

// search the name of device wether is success insmod
int find_module_devices(char *name)
{
    char *dirname =(char *) "/sys/class/input";
    char buf[256];
    char device_name[256];
    char classPath[256];
    int res;
    DIR *dir;
    struct dirent *de;
    int fd = -1;
    int ret = 0;

    memset(&device_name,0,sizeof(buf));
    memset(&buf,0,sizeof(buf));
    sprintf(device_name, "%s", name);

    dir = opendir(dirname);
    if (dir == NULL)
        return -1;

    while ((de = readdir(dir))) {
        memset(&buf,0,sizeof(buf));
        memset(&classPath,0,sizeof(classPath));
        if (strncmp(de->d_name, "input", strlen("input")) != 0) {
            continue;
        }

        sprintf(classPath, "%s/%s", dirname, de->d_name);
        snprintf(buf, sizeof(buf), "%s/name", classPath);

        fd = open(buf, O_RDONLY);
        if (fd < 0) {
            ALOGE("open %s failed %s \n",buf,strerror(errno));
            continue;
        }
        if ((res = read(fd, buf, sizeof(buf))) < 0) {
            close(fd);
            continue;
        }
        buf[res - 1] = '\0';

#ifdef DEBUG
        ALOGD("read input path buf:%s\n", buf);
#endif

        close(fd);
        fd = -1;
        ret = strcmp(device_name, buf);
        if (ret == 0) {
            ALOGE("success find device, name :%s device_name :%s read_name: %s\n",name, device_name, buf);
            return 0;
        }
    }

    closedir(dir);
    return -1;
}


//use to insmod single module
int insmodDevice(char *name, char *path)
{
    int fd;
    int ret;

    ALOGD("try insmod devices name = %s ,path = %s",name ,path);

    fd = TEMP_FAILURE_RETRY(open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (fd < 0) {
        ALOGE("open %s failed:%s", path, strerror(errno));
        return -1;
    }
    ret = syscall(__NR_finit_module, fd, "", 0);
    if (ret < 0) {
        ALOGE("finit_mdoule for %s failed:%s, fd = %d", path,
            strerror(errno), fd );
        return -1;
    }
    close(fd);
    ret = find_module_devices(name);
    if (ret < 0) {
        ALOGE("finit_mdoule for %s failed:%s, fd = %d", path,
                strerror(errno), fd );
        return -1;
    }

    return ret;
}

//use to insmod last success tp
int insmodLastDevice(void)
{
    char name_val[PROPERTY_VALUE_MAX];
    char path_val[PROPERTY_VALUE_MAX];
    property_get(PROPERTY_NAME, name_val, "");
    if (!strcmp(name_val, "")) {
        return -1;
    }

    property_get(PROPERTY_PATH, path_val, "");
    if (!strcmp(path_val, "")) {
        return -1;
    }
    return insmodDevice(name_val, path_val);
}

//insmod all module in cfg until one success
int insmodAllDevices(void)
{
    FILE *fp;
    int ret;
    char buf[LINE_LENGTH] = {0};
    char *name;
    char *path;

    memset(&buf,0,sizeof(buf));
    if ((fp = fopen(TP_MODULES_CFG, "r")) == NULL) {
        ALOGE("can't not open file!\n");
        return 0;
    }

    while (fgets(buf, LINE_LENGTH , fp)) {
#ifdef DEBUG
        ALOGD("read buf:%s\n",buf);
#endif
        if  (!is_alpha(buf[0])) {
            if (!strncmp(buf, MODULE_NAME, 4)) {
                name = get_module_name(buf);
            } else if (!strncmp(buf, MODULE_PATH, 4)) {
                path = get_modulepath(buf);
                ret = insmodDevice(name, path);

                if (ret == 0) {
                    property_set(PROPERTY_NAME, name);
                    property_set(PROPERTY_PATH, path);
                    break;
                }
                ALOGD("can't insmod device driver,name = %s,path = %s, maybe have not this device\n", name, path);
            }
        }
        memset(&buf, 0, sizeof(buf));
    }
    fclose(fp);
    return 0;
}

int main()
{
    int ret = insmodLastDevice();
    if (ret < 0) {
        property_set("persist.vendor.tp.name", "");
        property_set("persist.vendor.tp.path", "");
        ret = insmodAllDevices();
    }
}
