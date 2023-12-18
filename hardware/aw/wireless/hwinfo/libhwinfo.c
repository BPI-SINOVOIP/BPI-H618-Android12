/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/stat.h>
#include <libhwinfo.h>
#define LOG_TAG "WifiHWInfo"
#include <log/log.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <regex.h>

#define ARRAY_SIZE(arr)         (sizeof(arr) / sizeof((arr)[0]))
#define UEVENT_MSG_LEN          1024
#define WIFI_SCAN_DEVICE_PATH   "/sys/devices/virtual/misc/sunxi-wlan/rf-ctrl/scan_device"
#define WIFI_POWER_STATE_PATH   "/sys/devices/virtual/misc/sunxi-wlan/rf-ctrl/power_state"
#define MODULE_MODULE_INFO_PROP PROP_PREFIX "module_info"
#define MODULE_WLAN_VENDOR_PROP PROP_PREFIX "wlan_vendor"
#define MODULE_BT_VENDOR_PROP   PROP_PREFIX "bluetooth_vendor"
#define MODULE_BT_SUPPORT_PROP  PROP_PREFIX "bluetooth_support"

#define LINE_MAX                1024
#define INFO_SESSION_MAX        10
#define INFO_CONFIG_FILE        "/vendor/etc/hwinfo.conf"
#define CONFIG_SESSION_KEY      "[module]"

#define TYPE_ULONG              0
#define TYPE_PCHAR              1
#define TYPE_SINT               2

#if __SIZEOF_POINTER__ == 4
#define ADDR_T                  uint32_t
#else
#define ADDR_T                  uint64_t
#endif

#define MATCH_NODE(data_type, node_type, node) {#node, node_type, offsetof(data_type, node)}

struct match_t {
    char *keyname;
    int  type;
    int  offset;
};

struct info_t {
    char *vendor;
    char *info[INFO_SESSION_MAX];
};

struct wifi_hardware_info {
    unsigned long bus_type;
    unsigned long device_id;
    char *module_name;
    char *driver_name;
    char *driver_module_name;
    char *vendor_name;
    int  bt_support;
};

struct wl_hwinfo_list {
    struct wifi_hardware_info hwinfo;
    struct wl_hwinfo_list *next;
};

static const struct match_t matchtab[] = {
    MATCH_NODE(struct wifi_hardware_info, TYPE_ULONG, bus_type),
    MATCH_NODE(struct wifi_hardware_info, TYPE_ULONG, device_id),
    MATCH_NODE(struct wifi_hardware_info, TYPE_PCHAR, module_name),
    MATCH_NODE(struct wifi_hardware_info, TYPE_PCHAR, driver_name),
    MATCH_NODE(struct wifi_hardware_info, TYPE_PCHAR, driver_module_name),
    MATCH_NODE(struct wifi_hardware_info, TYPE_PCHAR, vendor_name),
    MATCH_NODE(struct wifi_hardware_info, TYPE_SINT,  bt_support),
};

static const struct info_t wifi_hal_name[] = {
    {"broadcom", {"libwifi-hal-bcm.so"   }},
    {"realtek",  {"libwifi-hal-rtk.so"   }},
    {"xradio",   {"libwifi-hal-xradio.so"}},
    {"qualcomm", {"libwifi-hal-qcom.so"  }},
    {"ssv",      {"libwifi-hal-ssv.so"   }},
    {"atbm",     {"libwifi-hal-atbm.so"  }},
    {"sprd",     {"libwifi-hal-sprd.so"  }},
    {"aic",      {"libwifi-hal-aic.so"   }},
};

static const struct info_t libbt_name[] = {
    {"broadcom", {"libbt-broadcom.so"}},
    {"realtek",  {"libbt-realtek.so" }},
    {"xradio",   {"libbt-xradio.so"  }},
    {"qualcomm", {"libbt-qualcomm.so"}},
    {"sprd",     {"libbt-sprd.so"    }},
    {"aic",      {"libbt-aic.so"     }},
};

static const struct info_t wifi_drv_para[] = {
    {"broadcom",
        {"nvram_path=/vendor/etc/firmware/nvram_${module_name}.txt",
         "config_path=/vendor/etc/firmware/config_${module_name}.txt",
         0,
        }
    },
    {"realtek",
        {"ifname=wlan0 if2name=p2p0",
         0,
        }
    },
    {"ssv",
        {"stacfgpath=/vendor/etc/firmware/${module_name}/${module_name}-wifi.cfg",
         0,
        }
    },
};

static const struct wifi_hardware_info wifi_list[] = {
/*  DeviceID  ModuleName      DriverName DriverModuleName  VendorName  BtSupport */
    {0x1, 0x024c8179, "rtl8189es",     "8189es",     "8189es",     "realtek",  0},
#ifdef WIFI_USE_RTL8723BS_VQ0
    {0x1, 0x024cb723, "rtl8723bs_vq0", "8723bs-vq0", "8723bs_vq0", "realtek",  1},
#else
    {0x1, 0x024cb723, "rtl8723bs",     "8723bs",     "8723bs",     "realtek",  1},
#endif
    {0x1, 0x024cb703, "rtl8723cs",     "8723cs",     "8723cs",     "realtek",  1},
    {0x1, 0x024cd723, "rtl8723ds",     "8723ds",     "8723ds",     "realtek",  1},
    {0x1, 0x024cf179, "rtl8189fs",     "8189fs",     "8189fs",     "realtek",  0},
    {0x1, 0x024cb822, "rtl88x2bs",     "88x2bs",     "88x2bs",     "realtek",  1},
    {0x0, 0x0bdad723, "rtl8723du",     "8723du",     "8723du",     "realtek",  1},
    {0x0, 0x0bda8179, "rtl8188etv",    "8188eu",     "8188eu",     "realtek",  0},
    {0x0, 0x0bda0179, "rtl8188eu",     "8188eu",     "8188eu",     "realtek",  0},
    {0x0, 0x0bda818b, "rtl8192eu",     "8192eu",     "8192eu",     "realtek",  0},
    {0x0, 0x0bdab720, "rtl8723bu",     "8723bu",     "8723bu",     "realtek",  1},
    {0x0, 0x0bdac82c, "rtl8822cu",     "88x2cu",     "88x2cu",     "realtek",  1},
    {0x0, 0x0bdaf179, "rtl8188fu",     "8188fu",     "8188fu",     "realtek",  0},
    {0x1, 0x02d0a9a6, "ap6212",        "bcmdhd",     "bcmdhd",     "broadcom", 1},
    {0x1, 0x02d04330, "ap6330",        "bcmdhd",     "bcmdhd",     "broadcom", 1},
    {0x1, 0x02d04356, "ap6356s",       "bcmdhd",     "bcmdhd",     "broadcom", 1},
    {0x1, 0x02d0a9bf, "ap6255",        "bcmdhd",     "bcmdhd",     "broadcom", 1},
    {0x1, 0x00202281, "xr819",         "xr819",      "xr819",      "xradio",   0},
    {0x1, 0x0a9e2282, "xr829",         "xr829",      "xr829",      "xradio",   1},
    {0x1, 0x0271050a, "qca6174a",      "qualcomm",   "qualcomm",   "atheros",  0},
    {0x1, 0x30303030, "ssv6x5x",       "ssv6x5x",    "ssv6x5x",    "ssv",      0},
    {0x1, 0x00000000, "uwe5622",       "sprdwl_ng",  "sprdwl_ng",  "sprd",     1},
    {0x1, 0x54490145, "aic8800",     "aic8800_fdrv","aic8800_fdrv","aic",      1},
    {0x1, 0xc8a1c08d, "aic8800",     "aic8800_fdrv","aic8800_fdrv","aic",      1},
    {0x0, 0xa69c8800, "aic8800",     "aic8800_fdrv","aic8800_fdrv","aic",      1},
    {0x0, 0x007a8888, "atbm6023is",  "atbm602x_wifi_usb","atbm602x_wifi_usb","atbm", 0},
};

/* default select invalid if get wifi_hardware_info failed */
static struct wifi_hardware_info hwinfo_default = {
/*  DeviceID  ModuleName      DriverName DriverModuleName  VendorName  BtSupport */
     0x1, 0xFFFFFFFF, "invalid",       "invalid",    "invalid",    "none",     0};

static enum {
    running,
    exiting,
    exited
} thread_state = exited;

static struct wl_hwinfo_list *head = NULL;

static int search_key(const char *str, const char *key)
{
    const char *p = NULL;
    int keysize = 0;
    int bufsize = 0;

    if (str == NULL || key == NULL) return -1;

    keysize = strlen(key);
    bufsize = strlen(str);
    if (bufsize < keysize) return -1;

    p = str;
    while (*p != 0 && *p == ' ') p++;

    if (*p == '#') return -1;

    if (str + bufsize - p < keysize) return -1;

    if (strncmp(p, key, keysize) != 0) return -1;

    p += keysize;

    while (*p != 0 && *p == ' ') p++;

    if (*p != 0 && *p != '\r' && *p != '\n') return -1;

    return 0;
}

static int search_key_val(const char *str, const char *key, char *val)
{
    const char *p = NULL;
    const char *dst = NULL;
    int keysize = 0;
    int bufsize = 0;

    if (str == NULL || key == NULL || val == NULL) return -1;

    keysize = strlen(key);
    bufsize = strlen(str);
    if (bufsize <= keysize) return -1;

    p = str;
    while (*p != 0 && *p == ' ') p++;

    if (*p == '#') return -1;

    if (str + bufsize - p <= keysize) return -1;

    if (strncmp(p, key, keysize) != 0) return -1;

    p += keysize;

    while (*p != 0 && *p == ' ') p++;

    if (*p != '=') return -1;

    p++;
    while (*p != 0 && *p == ' ') p++;

    if (*p == '"') p++;

    dst = p;
    while (*p != 0) p++;

    p--;
    while (*p == ' ') p--;

    if (*p == '"') p--;

    while (*p == '\r' || *p == '\n') p--;

    p++;
    strncpy(val, dst, p -dst);
    val[p - dst] = 0;
    return 0;
}

// You must free the result if result is non-NULL.
static char *str_replace(char *orig, char *rep, char *with)
{
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

static int str_expand(char *text, char *expand)
{
    static const char *pattern = "\\$\\{[^}]+\\}";
    regex_t reg;
    const size_t nmatch = 1;
    int count = 0, status;
    regmatch_t pmatch[1];
    char matchstr[32];
    char env_name[32];
    char *env_val = NULL;
    char *orig = text;
    char *out;

    status = regcomp(&reg, pattern, REG_EXTENDED);
    while (regexec(&reg, orig, nmatch, pmatch, 0) == 0) {
        memcpy(matchstr, orig + pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so);
        matchstr[pmatch[0].rm_eo - pmatch[0].rm_so] = 0;

        memcpy(env_name, orig + pmatch[0].rm_so + 2, pmatch[0].rm_eo - pmatch[0].rm_so - 3);
        env_name[pmatch[0].rm_eo - pmatch[0].rm_so - 3] = 0;

        env_val = getenv(env_name);
        out = str_replace(orig, matchstr, env_val);

        if (out) {
            memcpy(expand, out, strlen(out));
            expand[strlen(out)] = 0;
            free(out);
            orig = expand;
        }
        count++;
    }
    regfree(&reg);

    if (count == 0)
        memcpy(expand, text, strlen(text));

    return 0;
}

static struct wl_hwinfo_list *load_config_from_file(const char *filename)
{
    FILE *fp   = NULL;
    char *cstr = NULL;
    uint32_t i = 0;
    char conf[256];
    char buf[LINE_MAX];
    static struct wl_hwinfo_list *head = NULL;
    struct wl_hwinfo_list *new  = NULL;
    struct wl_hwinfo_list *cur  = NULL;

    if ((fp = fopen(filename, "r")) == NULL) {
        ALOGE("Open config file %s error!\n", filename);
        return head;
    }

    while (fgets(buf, LINE_MAX, fp)) {
        if (search_key(buf, CONFIG_SESSION_KEY) == 0) {
            new = (struct wl_hwinfo_list *)malloc(sizeof(struct wl_hwinfo_list));
            memset(new, 0, sizeof(struct wl_hwinfo_list));
            if (head == NULL) {
                head = new;
                cur  = new;
            } else {
                cur = head;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = new;
                cur       = cur->next;
            }
            continue;
        }

        for (i = 0; i < ARRAY_SIZE(matchtab); i++) {
            if (search_key_val(buf, matchtab[i].keyname, conf) == 0) {
                switch (matchtab[i].type) {
                    case TYPE_ULONG:
                    case TYPE_SINT:
                        *(ADDR_T *)((ADDR_T)&cur->hwinfo + matchtab[i].offset) = strtoul(conf, NULL, 0);
                        break;
                    case TYPE_PCHAR:
                        cstr = (char *)malloc(strlen(conf) + 1);
                        strcpy(cstr, conf);
                        *(ADDR_T *)((ADDR_T)&cur->hwinfo + matchtab[i].offset) = (ADDR_T)cstr;
                        break;
                    default:
                        break;
                }
                break;
            }
        }
    }
    fclose(fp);
    return head;
}

static struct wl_hwinfo_list *load_config_from_local(const struct wifi_hardware_info *list, uint32_t num)
{
    struct wifi_hardware_info *p = (struct wifi_hardware_info *)list;
    static struct wl_hwinfo_list *head = NULL;
    struct wl_hwinfo_list *new  = NULL;
    struct wl_hwinfo_list *cur  = NULL;
    char  *cstr = NULL;

    while (num--) {
        new = (struct wl_hwinfo_list *)malloc(sizeof(struct wl_hwinfo_list));
        memset(new, 0, sizeof(struct wl_hwinfo_list));
        if (head == NULL) {
            head = new;
            cur  = new;
        } else {
            cur = head;
            while (cur->next != NULL)
                cur = cur->next;
            cur->next = new;
            cur       = cur->next;
        }

        for (int i = 0; i < ARRAY_SIZE(matchtab); i++) {
            switch (matchtab[i].type) {
                case TYPE_ULONG:
                case TYPE_SINT:
                    *(ADDR_T *)((ADDR_T)&cur->hwinfo + matchtab[i].offset) = *(ADDR_T *)((ADDR_T)p + matchtab[i].offset);
                    break;
                case TYPE_PCHAR:
                    cstr = (char *)malloc(strlen((char *)(*(ADDR_T *)((ADDR_T)p + matchtab[i].offset))) + 1);
                    strcpy(cstr, (char *)(*(ADDR_T *)((ADDR_T)p + matchtab[i].offset)));
                    *(ADDR_T *)((ADDR_T)&cur->hwinfo + matchtab[i].offset) = (ADDR_T)cstr;
                    break;
                default:
                    break;
            }
        }
        p++;
    }
    return head;
}

static struct wl_hwinfo_list *load_merged_config(void)
{
    struct wl_hwinfo_list *h1   = load_config_from_local(wifi_list, ARRAY_SIZE(wifi_list));
    struct wl_hwinfo_list *h2   = load_config_from_file(INFO_CONFIG_FILE);
    struct wl_hwinfo_list *p1   = NULL;
    struct wl_hwinfo_list *p2   = NULL;
    struct wl_hwinfo_list *head = NULL;
    uint32_t delete_node = 0;

    if (h1 == NULL) {
        head = h2;
        goto merge;
    }

    head = h1;
    p1   = head;
    while (p1->next != NULL)
        p1 = p1->next;

    p1->next = h2;

merge:
    p1 = head;
    h1 = head;
    h2 = head;
    while (p1 != NULL && p1->next != NULL) {
        p2 = p1->next;
        while (p2 != NULL) {
            if (p1->hwinfo.device_id == p2->hwinfo.device_id) {
                delete_node = 1;
                p2 = p2->next;
                break;
            }
            p2 = p2->next;
        }

        p1 = p1->next;
        if (delete_node) {
            delete_node = 0;
            if (h2 == head) {
                h1 = p1;
                head = h1;
            } else {
                h1->next = p1;
            }
            for (int i = 0; i < ARRAY_SIZE(matchtab); i++) {
                if (matchtab[i].type == TYPE_PCHAR)
                    free((void *)(*(ADDR_T *)((ADDR_T)&h2->hwinfo + matchtab[i].offset)));
            }
            free(h2);
        } else {
            h1 = h2;
        }
        h2 = p1;
    }
    return head;
}

static struct wifi_hardware_info *get_hardware_info_by_device_id(const unsigned long bus_type, const unsigned long device_id)
{
    struct wl_hwinfo_list *cur = head;

    while (cur != NULL) {
        if (cur->hwinfo.bus_type == bus_type && cur->hwinfo.device_id == device_id) {
            return &cur->hwinfo;
        }
        cur = cur->next;
    }
    return NULL;
}

static int wifi_power_on(void)
{
    int fd = 0;
    int size = 0;
    char store_state = 0;
    char to_write = '1';

    fd = open(WIFI_POWER_STATE_PATH, O_RDWR);
    if (fd < 0) {
        ALOGE("%s error:%d %s###", __FUNCTION__, errno, strerror(errno));
        return -1;
    }

    size = read(fd, &store_state, sizeof(store_state));
    if (size <= 0) {
        close(fd);
        return -1;
    }

    if ((store_state - '0') > 0)
        return 1;

    size = write(fd, &to_write, sizeof(to_write));
    if (size < 0) {
        close(fd);
        return -1;
    }

    size = read(fd, &store_state, sizeof(store_state));
    if (size <= 0) {
        close(fd);
        return -1;
    }

    close(fd);
    return (store_state - '0');
}

static int wifi_power_off(void)
{
    int fd = 0;
    int size = 0;
    char to_write = '0';

    fd = open(WIFI_POWER_STATE_PATH, O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    size = write(fd, &to_write, sizeof(to_write));
    if (size < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int wifi_scan_device(int val)
{
    int fd = 0;
    int size = 0;
    char to_write = val ? '1' : '0';

    fd = open(WIFI_SCAN_DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    size = write(fd, &to_write, sizeof(to_write));
    if (size < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static struct wifi_hardware_info *parse_uevent(char *msg)
{
    char busdev_id[10] = {0};
    char uevent_id[10] = {0};
    char device_type[10] = {0};
    char *subsystem = NULL;
    char *sdio_id = NULL;
    char *usb_product = NULL;
    unsigned long device_id = 0;
    int bus_type = -1;
    ssize_t  cnt = 0;
    struct wifi_hardware_info *wifiinfo = NULL;

    while (*msg) {
        if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            subsystem = msg;
        } else if (!strncmp(msg, "SDIO_ID=", 8)) {
            msg += 8;
            strncpy(uevent_id, msg, 9);
        } else if (!strncmp(msg, "PRODUCT=", 8)) {
            msg += 8;
            strncpy(uevent_id, msg, 9);
        }

        /* advance to after the next \0 */
        while (*msg++) {
            /* do nothing */
        }
    }

    if (!strncmp(subsystem, "sdio", 4)) {
        ALOGI("get uevent, sdio_id = %s", sdio_id);
        strcpy(device_type, "sdio");
        char *subid = strtok(uevent_id, ":");
        if (subid == NULL)
            return NULL;
        cnt += sprintf(busdev_id + cnt, "%s", subid);
        subid = strtok(NULL, ":");
        if (subid == NULL)
            return NULL;
        cnt += sprintf(busdev_id + cnt, "%s", subid);
        device_id = strtoul(busdev_id, NULL, 16);
        bus_type  = 1;
    } else if (!strncmp(subsystem, "usb", 3)) {
        strcpy(device_type, "usb");
        char *subid = strtok(uevent_id, "/");
        if (subid == NULL)
            return NULL;
        cnt += sprintf(busdev_id + cnt, "%s", subid);
        subid = strtok(NULL, "/");
        if (subid == NULL)
            return NULL;
        cnt += sprintf(busdev_id + cnt, "%s", subid);
        device_id = strtoul(busdev_id, NULL, 16);
        bus_type  = 0;
    } else {
        return wifiinfo;
    }

    ALOGI("parse_uevent(), device_id: 0x%08lx", device_id);
    if ((wifiinfo = get_hardware_info_by_device_id(bus_type, device_id)) != NULL) {
        thread_state = exiting;
    }
    return wifiinfo;
}

static void *ls_device_thread(void *args)
{
    struct wifi_hardware_info *wifiinfo = NULL;
    ADDR_T *p = (ADDR_T *)args;
    char buf[UEVENT_MSG_LEN + 2] = {0};
    int count;
    int err;
    int retval;
    struct sockaddr_nl snl;
    int sock;
    struct pollfd fds;
    const int buffersize = 32*1024;

    thread_state = running;
    memset(&snl, 0x0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = 0;
    snl.nl_groups = 0xffffffff;
    sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (sock < 0) {
        ALOGE("####socket is failed in %s error:%d %s###", __FUNCTION__, errno, strerror(errno));
        return((void *)-1);
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));
    retval = bind(sock, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if (retval < 0) {
        ALOGE("####bind is failed in %s error:%d %s###", __FUNCTION__, errno, strerror(errno));
        close(sock);
       return((void *)-1);
    }

    while (running == thread_state) {
        fds.fd = sock;
        fds.events = POLLIN;
        fds.revents = 0;
        err = poll(&fds, 1, 1000);
        memset(buf, '\0', sizeof(char) * 1024);
        if (err > 0 && (fds.revents & POLLIN)) {
            count = recv(sock, buf, sizeof(char) * 1024,0);
            if (count > 0) {
                wifiinfo = parse_uevent(buf);
                *p = (ADDR_T)wifiinfo;
            }
        }
    }

    close(sock);
    thread_state = exited;
    return((void *)0);
}

static struct wifi_hardware_info *do_coldboot(DIR *d)
{
    struct dirent *de;
    int dfd, dfd2, fd, fd2;
    struct wifi_hardware_info *wifiinfo = NULL;

    dfd = dirfd(d);

    while ((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if (fd < 0)
            continue;

        d2 = fdopendir(fd);
        if (d2 == 0)
            close(fd);
        else {
            dfd2 = dirfd(d2);
            fd2 = openat(dfd2, "uevent", O_RDONLY);
            if (fd >= 0) {
                char *buf = NULL, *buf_tmp = NULL, *subid = NULL;
                char busdev_id[10] = {0};
                char uevent_id[10] = {0};
                unsigned long device_id;
                int bus_type = -1;
                ssize_t cnt = 0;

                buf = buf_tmp = malloc(UEVENT_MSG_LEN + 2);
                memset(buf, 0 , UEVENT_MSG_LEN + 2);

                read(fd2, buf, UEVENT_MSG_LEN);
                while (*buf) {
                    /* SDIO BUS device, SDIO_ID=0000:0000 */
                    if (!strncmp(buf, "SDIO_ID=", 8)) {
                        buf += 8;
                        strncpy(uevent_id, buf, 9);
                        ALOGD("read from sdio path, sdio_id=%s", uevent_id);
                        subid = strtok(uevent_id, ":");
                        if (subid == NULL)
                            break;
                        cnt += sprintf(busdev_id + cnt, "%s", subid);
                        subid = strtok(NULL, ":");
                        if (subid == NULL)
                            break;
                        cnt += sprintf(busdev_id + cnt, "%s", subid);
                        device_id = strtoul(busdev_id, NULL, 16);
                        bus_type  = 1;
                        break;
                    }

                    /* USB BUS device, PRODUCT=bda/d723/200 */
                    if (!strncmp(buf, "PRODUCT=", 8)) {
                        buf += 8;
                        strncpy(uevent_id, buf, 9);
                        ALOGD("read from usb path, usb_id=%s", uevent_id);
                        subid = strtok(uevent_id, "/");
                        if (subid == NULL)
                            break;
                        cnt += sprintf(busdev_id + cnt, "%s", subid);
                        subid = strtok(NULL, "/");
                        if (subid == NULL)
                            break;
                        cnt += sprintf(busdev_id + cnt, "%s", subid);
                        device_id = strtoul(busdev_id, NULL, 16);
                        bus_type  = 0;
                        break;
                    }
                    buf++;
                }
                close(fd2);
                free(buf_tmp);
                if (bus_type != -1 && (wifiinfo = get_hardware_info_by_device_id(bus_type, device_id)) != NULL) {
                    closedir(d2);
                    return wifiinfo;
                }
            }
            closedir(d2);
        }
    }
    return NULL;
}

static struct wifi_hardware_info *coldboot(const char *path)
{
    struct wifi_hardware_info *wifiinfo = NULL;
    DIR *d = opendir(path);

    if (d) {
        wifiinfo = do_coldboot(d);
        closedir(d);
    }
    return wifiinfo;
}

static struct wifi_hardware_info *get_wifi_hardware_info(void)
{
    char info[PROPERTY_VALUE_MAX] = {0};
    char *pinfo = info;
    pthread_t ls_device_thread_fd;
    int ret = 0;
    int i   = 0;
    int store_power_state = 0;
    unsigned long device_id, bus_type;
    ADDR_T info_addr = 0;
    struct wl_hwinfo_list *cur = NULL;
    static struct wifi_hardware_info *wifiinfo = NULL;

    if (wifiinfo == &hwinfo_default)
        goto detect;

    if (wifiinfo == NULL) {

        ALOGD("Start to load wireless module hardware info table...");
        head = load_merged_config();
        cur  = head;
        while (cur != NULL) {
            i++;
            ALOGD("Entry [%02d], BUS: %4s, ID: 0x%08lX, Name: %s.", i, (cur->hwinfo.bus_type == 0) ? "USB" : "SDIO",
                        cur->hwinfo.device_id, cur->hwinfo.module_name);
            cur = cur->next;
        }

        if (property_get(MODULE_MODULE_INFO_PROP, info, NULL) == 0)
            goto detect;

        pinfo = strchr(pinfo, ':');
        if (!pinfo)
            goto detect;

        pinfo++;
        pinfo = strchr(pinfo, ':');
        if (!pinfo)
            goto detect;

        pinfo++;
        bus_type = strtoul(pinfo, NULL, 0);

        pinfo = strchr(pinfo, ':');
        if (!pinfo)
            goto detect;

        pinfo++;
        device_id = strtoul(pinfo, NULL, 0);
        if ((wifiinfo = get_hardware_info_by_device_id(bus_type, device_id)) != NULL)
            goto done;

detect:
        property_set(MODULE_MODULE_INFO_PROP, "");
        ALOGD("%s not exist, try to create it!", MODULE_MODULE_INFO_PROP);
        if ((wifiinfo = coldboot("/sys/bus/sdio/devices")) != NULL)
            goto done;

        if ((wifiinfo = coldboot("/sys/bus/usb/devices")) != NULL)
            goto done;

        ret = pthread_create(&ls_device_thread_fd, NULL, ls_device_thread, &info_addr);
        if (ret) {
            ALOGE("Create ls_device_thread error!\n");
            goto done;
        }

        for (i = 0; (i++ < 10) && (store_power_state <= 0); i++) {
            if ((store_power_state = wifi_power_on()) == 1)
                break;
            usleep(100000);
        }

        if (store_power_state <= 0)
            goto done;

        wifi_scan_device(1);

        for (i = 0; i < 20; i++) {
            if (exited == thread_state)
                break;
            usleep(100000);
        }

        if (running == thread_state) {
            ALOGE("get uevent timeout!\n");
            thread_state = exiting;
        }

        wifi_power_off();
        wifi_scan_device(0);
        wifiinfo = (struct wifi_hardware_info *)info_addr;

done:
        if (wifiinfo == NULL) {
            ALOGE("Cannot find matched module, use default!");
            wifiinfo = &hwinfo_default;
            return wifiinfo;
        }

        if (property_get(MODULE_MODULE_INFO_PROP, info, NULL) == 0) {
            snprintf(info, sizeof(info), "%s:%s:0x%lx:0x%08lx", wifiinfo->vendor_name, wifiinfo->module_name, wifiinfo->bus_type, wifiinfo->device_id);
            property_set(MODULE_MODULE_INFO_PROP, info);
            property_set(MODULE_WLAN_VENDOR_PROP, wifiinfo->vendor_name);
            property_set(MODULE_BT_VENDOR_PROP,   wifiinfo->bt_support ? wifiinfo->vendor_name : "NULL");
            property_set(MODULE_BT_SUPPORT_PROP,  wifiinfo->bt_support ? "1" : "0");
        }
    }

    for (i = 0; i < ARRAY_SIZE(matchtab); i++) {
        if (matchtab[i].type == TYPE_PCHAR)
            setenv(matchtab[i].keyname, (char *)(*(ADDR_T *)((ADDR_T)wifiinfo + matchtab[i].offset)), 1);
    }

    return wifiinfo;
}

const char *get_wifi_vendor_name(void)
{
    struct wifi_hardware_info *hwinfo = get_wifi_hardware_info();
    return hwinfo->vendor_name;
}

const char *get_wifi_module_name(void)
{
    struct wifi_hardware_info *hwinfo = get_wifi_hardware_info();
    return hwinfo->module_name;
}

const char *get_wifi_driver_name(void)
{
    struct wifi_hardware_info *hwinfo = get_wifi_hardware_info();
    return hwinfo->driver_name;
}

const char *get_wifi_driver_module_name(void)
{
    struct wifi_hardware_info *hwinfo = get_wifi_hardware_info();
    return hwinfo->driver_module_name;
}

const char *get_driver_module_arg(void)
{
    static char module_arg[1024] = {0};
    struct wifi_hardware_info *hwinfo = get_wifi_hardware_info();
    char   buffer[1024];
    int    n = 0;
    for (int i = 0; i < ARRAY_SIZE(wifi_drv_para); i++) {
        if (strncmp(hwinfo->vendor_name, wifi_drv_para[i].vendor, strlen(hwinfo->vendor_name)) == 0) {
            for (int j = 0; (j < INFO_SESSION_MAX) && wifi_drv_para[i].info[j] != NULL; j++) {
                n += sprintf(&buffer[n], "%s ", wifi_drv_para[i].info[j]);
            }
            buffer[n - 1] = '\0';
            str_expand(buffer, module_arg);
            return module_arg;
        }
    }
    return module_arg;
}

const char *get_wifi_hal_name(void)
{
    const char *vendor_name = get_wifi_vendor_name();
    for (int i = 0; i < ARRAY_SIZE(wifi_hal_name); i++) {
        if (strncmp(vendor_name, wifi_hal_name[i].vendor, strlen(vendor_name)) == 0)
            return wifi_hal_name[i].info[0];
    }
    return NULL;
}

const char *get_bluetooth_libbt_name(void)
{
    const char *vendor_name = get_wifi_vendor_name();
    for (int i = 0; i < ARRAY_SIZE(libbt_name); i++) {
        if (strncmp(vendor_name, libbt_name[i].vendor, strlen(vendor_name)) == 0)
            return libbt_name[i].info[0];
    }
    return NULL;
}

int get_bluetooth_is_support(void)
{
    struct wifi_hardware_info *hwinfo = get_wifi_hardware_info();
    return hwinfo->bt_support;
}
