
#ifndef __MULTI_IR_H__
#define __MULTI_IR_H__

#include <linux/ioctl.h>

/* ioctl cmd define */
#define MULTI_IR_IOC_MAGIC    'M'
/* return the max mapping table count support, if error, return -1 */
#define MULTI_IR_IOC_REQ_MAP    _IOR(MULTI_IR_IOC_MAGIC, 1, int *)
#define MULTI_IR_IOC_SET_MAP    _IOW(MULTI_IR_IOC_MAGIC, 2, void *)
#define MULTI_IR_IOC_CLR_MAP    _IOW(MULTI_IR_IOC_MAGIC, 3, int)

#define MAX_NAME_LEN        (32)
#define KEYCODE_MIN            (0)
#define KEYCODE_MAX            (300)
#define KEYCODE_CNT            (KEYCODE_MAX-KEYCODE_MIN+1)
#define KEY_MAPING_MAX        (18)

#define RESERVE_CODE    0
#define RESERVE_NAME    "!RESERVE"


/*
 A mapping table is mapping from customer ir key layout file(*.kl)
 to default layout file.

 For example:
    in coustomer kl file  -->  key 28  POWER
    in default kl file    -->  key 57  POWER

    then, the mapping will be like that: mapping_table.value[57] = 28
*/
struct mapping_table_t {
    int identity;                /* means ir address */
    int powerkey;                /* powerky to wakeup system */
    int value[KEYCODE_CNT];        /* convert from coustomer keycode to
                               default keycode */
};

void setMouseMode(int mode);
int report_mouse_keyevent(int scan_code, int key_state);
void set_ir_state(int state);

#define HDMI_CEC_STANDBY_SUPPORT

#ifdef HDMI_CEC_STANDBY_SUPPORT
#define POLL_FD_NUM        (3)
#define CEC_FD_INDEX    (1)
#define INPUT_FD_INDEX    (2)
#else
#define POLL_FD_NUM        (2)
#define INPUT_FD_INDEX    (1)
#endif

#endif /* __MULTI_IR_H__ */
