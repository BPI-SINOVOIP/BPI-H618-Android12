/*
 * multi ir daemon for android -
 * Copyright (C) 2015-2018 AllwinnerTech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <thread>

#include "cutils/properties.h"
#ifndef BUILD_TARGET_RECOVERY
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "MultiirService.h"
#endif
#include "virtual_input.h"
#include "multiir.h"


#ifdef HDMI_CEC_STANDBY_SUPPORT
extern int uevent_init(int *ufds);
extern int uevent_process_event(void);
#endif

#define multi_debug(fmt, arg...)                \
do {                                            \
    if (mult_flags & PRINT_DEBUG)               \
        fprintf(stdout, "[D] " fmt"\n", ##arg); \
} while(0)

enum {
    PRINT_ERROR  = 1U << 0,
    PRINT_WARN   = 1U << 1,
    PRINT_DEBUG  = 1U << 2,
};

static int mult_flags = 0x0;

extern int g_mousefd;
int event_mode = 0;

pthread_mutex_t mutex;
struct itimerval timer_value;
//#define DEBUG

extern int nfds;
#define MOUSE_CODE  (0xE8)
#define DPAD_UP     19
#define DPAD_DOWN   20
#define DPAD_LEFT   21
#define DPAD_RIGHT  22
#define DPAD_CENTER 23

#define KEY_LAYOUT_PATH         "/system/usr/keylayout"
#define DEFAULT_KEYLAYOUT_FILE  "sunxi-ir.kl"
#define IR_KEYPRESS_TIMEOUT      (500000)
#define IR_REPEAT_PERIOD         (80)
#define DEFAULT_POINTER_SPEED    IR_REPEAT_PERIOD
#define POINTER_SPEED_MAX        (100)
#define POINTER_SPEED_MIN        (10)
#define DEFAULT_STEP_DISTANCE    20

static int pointer_speed = DEFAULT_POINTER_SPEED;
static int mDistance = DEFAULT_STEP_DISTANCE;
static int mIRState = 1;

int get_default_pointerspeed(void)
{
    return POINTER_SPEED_MAX - DEFAULT_POINTER_SPEED;
}

int get_default_stepdistance(void)
{
    return DEFAULT_STEP_DISTANCE;
}

void set_pointerspeed(int ms)
{
    // real speed = POINTER_SPEED_MAX - ms
    // userspace set speed, value more smaller speed more faster
    if (ms < POINTER_SPEED_MIN)
        pointer_speed = POINTER_SPEED_MAX;
    else if (ms > POINTER_SPEED_MAX)
        pointer_speed = POINTER_SPEED_MIN;
    else
        pointer_speed = POINTER_SPEED_MAX - ms + POINTER_SPEED_MIN;
}

void set_stepdistance(int px)
{
    if (px < 5)
        mDistance = 5;
    else if (px > 50)
        mDistance = 50;
    else
        mDistance = px;
}

void reset(void)
{
    set_pointerspeed(DEFAULT_POINTER_SPEED);
    set_stepdistance(DEFAULT_STEP_DISTANCE);
}

void set_ir_state(int state)
{
    mIRState = state;
}

/*
 * A valid customer ir key layout file *MUST* namd by 'customer_ir_xxxx.kl',
 * which 'xxxx' is the identity(ir address code, in hex),
 * eg. customer_ir_9f00.kl
*/
#define FILENAME_PREFIX         "customer_ir_"
#define FILENAME_PREFIX_RC5     "customer_rc5_ir_"
#define FILENAME_EXTENSION      ".kl"

#define IR_PROTOCOL_USED        "/proc/sunxi_ir_protocol"
#define NEC (0)
#define RC5 (1)
static unsigned int  ir_protocol_used ;


struct keymap_t {
    int keycode;
    char name[MAX_NAME_LEN];
};

struct key_dev_t {
    int uinputfd;
    struct input_event *event;
    int last_code;
    int last_value;
    int keypressed;
};

struct keymap_t default_keys[KEYCODE_CNT];
struct keymap_t customer_keys[KEYCODE_CNT];
struct mapping_table_t mapping_table[KEY_MAPING_MAX];
struct key_dev_t sunxi_ir_dev;

int mapping_table_cnt = 0;

void keymap_init(struct keymap_t *map)
{
    int i;
    for (i=0; i<KEYCODE_CNT; i++, map++) {
        map->keycode = RESERVE_CODE;
        strcpy(map->name, RESERVE_NAME);
    }
}

#ifdef DEBUG
void dump_keymap(struct keymap_t *map)
{
    int i;

    for (i=0; i<KEYCODE_CNT; i++, map++) {
        if (!strlen(map->name)) continue;
        printf("key\t%d\t%s\n", map->keycode, map->name);
    }
}

void dump_mapping_table(struct mapping_table_t *table, struct keymap_t *def)
{
    int i;

    printf("+ identity: 0x%04x\n", table->identity);
    for (i=0; i<KEYCODE_CNT; i++) {
        printf("\t[%3d] --> { %3d, %-20s }\n", i, table->value[i],
                    (def + table->value[i])->name);
    }
}
#endif

static int isDPADKey(int scan) {
    int DPADKeyCode[4] = {DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT};
    int i;
    for (i = 0; i < 4; i++) {
        if (DPADKeyCode[i] == scan) return 1;
    }
    return 0;
}

/**
 * @brief: create a keymap form keylayout file(*.kl)
 * @param: path - path to the target file
 * @param: map - store the result
 * @return:
 */
int create_keymap_from_file(const char *path, struct keymap_t *map)
{
    int ret = 0;
    FILE *kl_fd = NULL;
    char buf[1024], lable[32], name[MAX_NAME_LEN];
    int keycode;
    char *p;
    struct keymap_t *des;

    kl_fd = fopen(path, "r");
    if (!kl_fd) {
        ALOGE("open '%s' fail, %s\n", path, strerror(errno));
        return -1;
    }

    while (fgets(buf, 1024, kl_fd)) {
        p = buf;
        while (*p==' ') p++;
        if (*p=='#') continue;

        if (sscanf(buf, "%31s %d %31s", lable, &keycode, name)!=3) continue;
        if (strcmp(lable, "key")!=0 || keycode < KEYCODE_MIN || keycode > KEYCODE_MAX) continue;

        des = (struct keymap_t *)(map + keycode);
        des->keycode = keycode;
        strcpy(des->name, name);
    }

    if (!feof(kl_fd)) {
        ALOGE("reading '%s' error, %s\n", path, strerror(errno));
        ret = -1;
    }

    fclose(kl_fd);
    return ret;
}

/**
 * @brief: make a mapping from src to des, the result is store at @table,
 *         the mapping will set to ir driver through ioctl syscall.
 * @param: src
 * @param: des
 * @param: table
 */
void generate_mapping_table(struct keymap_t* src, struct keymap_t *des, struct mapping_table_t *table)
{
    int i, j;
    struct keymap_t *p;

    memset(table, 0, sizeof(struct mapping_table_t));
    for (i=0; i<KEYCODE_CNT; i++, src++) {
        if (src->keycode==RESERVE_CODE) continue;

        p = des;
        for (j=0; j<KEYCODE_CNT; j++, p++) {
            if (p->keycode!=-1 && strcmp(src->name, p->name)==0) {
                table->value[p->keycode] = src->keycode;

                if (table->powerkey==0 && strcmp(src->name, "POWER")==0) {
                    table->powerkey = p->keycode;
                }

                p->keycode = -1;
            }
        }
    }
}

/**
 * @brief: filter out the valid kaylaout file.
 * @param: name
 * @return: -1 means something error, else the identity.
 */
int kl_filename_verify(const char *name)
{
    char *p;
    int i, identity;
    /* filename prefix filter */
    if(ir_protocol_used == RC5){
        if (strncmp(name, FILENAME_PREFIX_RC5, strlen(FILENAME_PREFIX_RC5)))
            return -1;
    }
    else if(ir_protocol_used == NEC){
        if (strncmp(name, FILENAME_PREFIX, strlen(FILENAME_PREFIX)))
            return -1;
    }
    /* filename extensiion filter */
    if(ir_protocol_used == RC5){
        p = (char *)name + strlen(FILENAME_PREFIX_RC5) + 2;
        if (strcmp(p, FILENAME_EXTENSION))
            return -1;
    }
    else if(ir_protocol_used == NEC){
        p = (char *)name + strlen(FILENAME_PREFIX) + 4;
        if (strcmp(p, FILENAME_EXTENSION))
            return -1;
    }

    /* identity filter, 4 hexadecimal number */
    if(ir_protocol_used == RC5){
        p = (char *)name + strlen(FILENAME_PREFIX_RC5);
        for (i=0; i<2; i++) {
            if (isxdigit(*(p+i))==0)
                return -1;
        }
    }
    else if(ir_protocol_used == NEC){
        p = (char *)name + strlen(FILENAME_PREFIX);
        for (i=0; i<4; i++) {
            if (isxdigit(*(p+i))==0)
                return -1;
        }
    }
    identity =  strtol(p, &p, 16);

    return identity;
}

int init_sunxi_ir_map(void)
{
    int mapping_table_cnt = 0;
    char kl_path[512] = {0};
    DIR *dir;
    struct dirent *dirent;
    int identity;

    /* create default key map */
    keymap_init(default_keys);
    sprintf(kl_path, "%s/%s", KEY_LAYOUT_PATH, DEFAULT_KEYLAYOUT_FILE);
    create_keymap_from_file(kl_path, default_keys);

#ifdef DEBUG
    dump_keymap(default_keys);
#endif

    dir = opendir(KEY_LAYOUT_PATH);
    if (!dir) {
        ALOGE("Open directory '%s' error, %s\n",
            KEY_LAYOUT_PATH, strerror(errno));
        exit(-1);
    }

    while ((dirent = readdir(dir))!= NULL) {
        identity = kl_filename_verify(dirent->d_name);
        if ((identity != -1) && (mapping_table_cnt < KEY_MAPING_MAX)) {
#ifdef DEBUG
            ALOGI("config filename %s\n", dirent->d_name);
#endif
            sprintf(kl_path, "%s/%s", KEY_LAYOUT_PATH, dirent->d_name);
            memset(customer_keys, 0, sizeof(struct keymap_t)*(KEYCODE_CNT));
            create_keymap_from_file(kl_path, customer_keys);
            generate_mapping_table(default_keys, customer_keys, &(mapping_table[mapping_table_cnt]));
            mapping_table[mapping_table_cnt].identity = identity;
            #ifdef DEBUG
                dump_mapping_table(&(mapping_table[mapping_table_cnt]), default_keys);
            #endif
            mapping_table_cnt++;
        }
    }
    closedir(dir);
    return mapping_table_cnt;
}

int detect_key_event(struct input_event *event, int ir_map_cnt)
{
    int i;
    int scan_code = 0;
    int scan_addr = 0;

    if(ir_protocol_used == RC5){
        scan_code = event->value & 0x7f;
        scan_addr = (event->value >> 8) & 0x1f;
    }
    else if(ir_protocol_used == NEC){
        scan_code = event->value & 0xff;
        scan_addr = (event->value >> 8) & 0xffff;
    }
    for(i = 0; i < ir_map_cnt; i++)
    {
        if(mapping_table[i].identity == scan_addr)
        {
            return mapping_table[i].value[scan_code];
        }
    }
    return -1;
}

int convertCenterEvent(int scancode, int sync)
{
    struct input_event temp_event;
    if (scancode != DPAD_CENTER)
        return 0;
    temp_event.code = BTN_LEFT;
    temp_event.type = EV_KEY;
    temp_event.value = 0;
    report_key_event(g_mousefd, &temp_event, sync);
    return 0;
}

int ir_key_up(struct key_dev_t *ir_dev, int code, int sync)
{
    struct input_event temp_event;

    if(!(ir_dev->keypressed))
    {
        return 0;
    }
    temp_event.type = EV_KEY;
    if (event_mode && (code == DPAD_CENTER))
        return convertCenterEvent(code, sync);
    temp_event.code = code;
    temp_event.value = 0;
    report_key_event(ir_dev->uinputfd, &temp_event, sync);
    multi_debug("up %04x %04x %08x\n", temp_event.type, temp_event.code, temp_event.value);
    ir_dev->keypressed = 0;
    return 0;
}

int convertEvent(struct key_dev_t *ir_dev, int scancode)
{
    struct input_event temp_event;
    temp_event.type = EV_REL;
    switch (scancode) {
        case DPAD_UP:
            temp_event.code = REL_Y;
            temp_event.value = -mDistance;
            break;
        case DPAD_DOWN:
            temp_event.code = REL_Y;
            temp_event.value = mDistance;
            break;
        case DPAD_LEFT:
            temp_event.code = REL_X;
            temp_event.value = -mDistance;
            break;
        case DPAD_RIGHT:
            temp_event.code = REL_X;
            temp_event.value = mDistance;
            break;
        case DPAD_CENTER:
            temp_event.type = EV_KEY;
            temp_event.code = BTN_LEFT;
            temp_event.value = 1;
            break;
        default:
            return 0;
    }
    ir_dev->keypressed = 1;
    ir_dev->last_code = temp_event.code;
    ir_dev->last_value = temp_event.value;
    report_key_event(g_mousefd, &temp_event, 1);
    return 0;
}

int ir_key_down(struct key_dev_t *ir_dev, int scancode)
{
    struct input_event temp_event;

    if (ir_dev->keypressed)
        ir_key_up(ir_dev, scancode, 1);

    if (event_mode && (isDPADKey(scancode) || scancode == DPAD_CENTER)) {
        return convertEvent(ir_dev, scancode);
    }

    temp_event.type = EV_KEY;
    temp_event.code = scancode;
    temp_event.value = 1;
    ir_dev->keypressed = 1;
    ir_dev->last_code = scancode;
    report_key_event(ir_dev->uinputfd, &temp_event, 1);
    multi_debug("down %04x %04x %08x\n", temp_event.type, temp_event.code, temp_event.value);
    return 0;
}

int sunxi_timer_init(struct itimerval *value, int repeat_num)
{
    if(repeat_num == 0)
    {
        value->it_value.tv_sec = 0;
        value->it_value.tv_usec = IR_KEYPRESS_TIMEOUT; //200ms定时
        value->it_interval.tv_sec = 0;
        value->it_interval.tv_usec = IR_KEYPRESS_TIMEOUT; //200ms定时
    }else if(repeat_num > 0) {  // Support to decrease the period of repeating time
        value->it_value.tv_sec = 0;
        value->it_value.tv_usec = pointer_speed * 1000;
        value->it_interval.tv_sec = 0;
        value->it_interval.tv_usec = pointer_speed * 1000;
    }
    setitimer(ITIMER_REAL, value, NULL);
    return 0;
}

void ir_key_repeat(int signum)
{
    struct input_event temp_event;

    if (signum!=SIGALRM)
        return;

    pthread_mutex_lock(&mutex);
    if(sunxi_ir_dev.keypressed)
    {
        temp_event.type = EV_REL;
        temp_event.code = sunxi_ir_dev.last_code;
        temp_event.value = sunxi_ir_dev.last_value;
        report_key_event(g_mousefd, &temp_event, 1);
        sunxi_timer_init(&timer_value, 1);
    }
    pthread_mutex_unlock(&mutex);
    return;
}

int sunxi_mod_timer(struct itimerval *value)
{
    setitimer(ITIMER_REAL, value, NULL);
    return 0;
}

int sunxi_del_timer(struct itimerval *value)
{
    value->it_value.tv_sec = 0;
    value->it_value.tv_usec = 0;
    value->it_interval.tv_sec = 0;
    value->it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, value, NULL);
    return 0;
}

#ifdef CEC_ONE_TOUCH_PLAY
#define ONE_TOUCH_PLAY_CODE (0xDF)
void trigger_one_touch_play(void)
{
    property_set("hdmi_cts.one_touch_play", "1");
}
#endif



void setMouseMode(int mode) {
    event_mode = mode;
}

int detect_key_input(struct key_dev_t *ir_dev)
{
    int scan_code;
    int key_state;
    if (mIRState == 0)
        return 0;

    scan_code = detect_key_event(ir_dev->event, mapping_table_cnt);

    if(scan_code <= 0)
    {
        return 0;
    }

    key_state = (ir_dev->event->value >> 24) & 0x1;

#ifdef CEC_ONE_TOUCH_PLAY
    if (key_state && scan_code == ONE_TOUCH_PLAY_CODE) {
        trigger_one_touch_play();
        ALOGD("trigger one touch play.");
    }
#endif

    if(key_state)
    {
        ir_key_down(ir_dev, scan_code);
        if(event_mode && isDPADKey(scan_code))
        {
            sunxi_timer_init(&timer_value, 0);
        }
    }
    else
    {
        if(event_mode)
        {
            sunxi_del_timer(&timer_value);
        }
        ir_key_up(ir_dev, scan_code, 1);
    }
    return 0;
}

static int _report_mouse_keyevent(struct key_dev_t *ir_dev, int scan_code, int key_state)
{
    if (scan_code <= 0) {
        return 0;
    }

    if (key_state) {
        ir_key_down(ir_dev, scan_code);
        if (event_mode && isDPADKey(scan_code)) {
            sunxi_timer_init(&timer_value, 0);
        }
    } else {
        if (event_mode) {
            sunxi_del_timer(&timer_value);
        }
        ir_key_up(ir_dev, scan_code, 1);
    }
    return 0;
}

int report_mouse_keyevent(int scan_code, int key_state)
{
    return _report_mouse_keyevent(&sunxi_ir_dev, scan_code, key_state);
}

static int  ir_protocol(unsigned int *ir_protocol_used )
{
    int res = 0,fd = 0;
    fd = open(IR_PROTOCOL_USED,O_RDONLY|O_NONBLOCK);
    res = read(fd,ir_protocol_used,sizeof(unsigned int));
    printf("IR_PROTOCOL_USED = %d\n",*ir_protocol_used);
    if(res < 0)
    {
        ALOGE("can not read IR_PROTOCOL_USED\n");
        return  1;
    }
    if((*ir_protocol_used!= NEC)&&(*ir_protocol_used!= RC5))
    {
        ALOGE("WRONG : sunxi only support rc5 or nec protocol now\n\n");
        return 1;
    }
    close(fd);
    return 0;
}
#ifndef BUILD_TARGET_RECOVERY
std::thread mServiceThread;
void startService(void)
{
    android::sp<android::ProcessState> proc(android::ProcessState::self());
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::MultiirService::instantiate();
    android::ProcessState::self()->startThreadPool();
    android::IPCThreadState::self()->joinThreadPool();
}
#endif

int main(void)
{
    int err;
    //int scan_code = 0;
    //const char *devname = NULL;
    const char *inotify_path = "/dev/input";
    char uinput_devname[32] = "sunxi-ir-uinput";
    char devpath[PATH_MAX];
    struct pollfd ufds[POLL_FD_NUM];
    char device_name[16] = {0};
    struct inputdev_info * sourcedev = NULL;
    struct input_event ir_event;
    int uinputfd = -1;
    //To determine which protocol of IR will be used
    if(ir_protocol(&ir_protocol_used))
    {
        ALOGE("WRONG:faied to get ir_protocol_used \
            default to use NEC protocol\n");
        ir_protocol_used = NEC;
    }
    //虚拟设备建立
    uinputfd = setup_virtual_input_dev(uinput_devname);
    ALOGD("uinputfd=%d\n", uinputfd);
    if(uinputfd <= 0)
    {
        return 0;
    }

    sunxi_ir_dev.uinputfd = uinputfd;
    sunxi_ir_dev.event = &ir_event;

    mapping_table_cnt = init_sunxi_ir_map();
    if(mapping_table_cnt <= 0)
    {
        ALOGE("cant find any ir map");
        return 0;
    }

    pthread_mutex_init(&mutex, NULL);
    signal(SIGALRM, ir_key_repeat);

    memset(ufds, 0, sizeof(ufds));
    memset(&sourcedev, 0, sizeof(sourcedev));

    /* inotify when '/dev/inpu't dir change */
    nfds = 1;
    ufds[0].fd = inotify_init();
    ufds[0].events = POLLIN;
    err = inotify_add_watch(ufds[0].fd, inotify_path, IN_DELETE | IN_CREATE);
    if (err < 0) {
        ALOGE("could not add watch for '%s', %s",
            inotify_path, strerror(errno));
        return -1;
    }

#ifdef HDMI_CEC_STANDBY_SUPPORT
    if (uevent_init(&ufds[CEC_FD_INDEX].fd)==0) {
        nfds++;
        ufds[CEC_FD_INDEX].events = POLLIN;
        ALOGD("uevent listen start");
    }
#endif

    strcpy(device_name, "sunxi-ir");

    /* try to find the target input device */
    err = try_to_find_device(device_name, devpath);
    if (err==0 && strlen(devpath)) {
        ALOGD("find target device, path=%s", devpath);
        sourcedev = open_device(devpath, ufds);
    }

#ifndef BUILD_TARGET_RECOVERY
    mServiceThread = std::thread(startService);
#endif

    while(1) {
        poll(ufds, nfds, -1);
        if (ufds[0].revents & POLLIN) {
            notify_process(sourcedev, uinput_devname, inotify_path, ufds[0].fd, ufds);
        }
        if (ufds[INPUT_FD_INDEX].revents & POLLIN) {

            if(!input_event_process(ufds[INPUT_FD_INDEX].fd, &ir_event))
            {
                detect_key_input(&sunxi_ir_dev);
            }
        }
        #ifdef HDMI_CEC_STANDBY_SUPPORT
        if (ufds[CEC_FD_INDEX].revents & POLLIN) {
            uevent_process_event();
        }
        #endif
    }
    return 0;
}
