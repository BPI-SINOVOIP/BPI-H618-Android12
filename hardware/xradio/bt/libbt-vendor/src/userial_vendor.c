/******************************************************************************
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      userial_vendor.c
 *
 *  Description:   Contains vendor-specific userial functions
 *
 ******************************************************************************/
#undef NDEBUG
#define LOG_TAG "bt_userial_vendor"

#include <utils/Log.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "bt_vendor_xr.h"
#include "userial.h"
#include "userial_vendor.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef VNDUSERIAL_DBG
#define VNDUSERIAL_DBG FALSE
#endif

#if (VNDUSERIAL_DBG == TRUE)
#define VNDUSERIALDBG(param, ...)   {ALOGD(param, ## __VA_ARGS__);}
#else
#define VNDUSERIALDBG(param, ...)   {}
#endif

#define UNUSED(x)                   (void)(x)

#define VND_PORT_NAME_MAXLEN        256

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* vendor serial control block */
typedef struct {
    int fd;                     /* fd to Bluetooth device */
    struct termios termios;     /* serial terminal of BT port */
    char port_name[VND_PORT_NAME_MAXLEN];
} vnd_userial_cb_t;

/******************************************************************************
**  Static variables
******************************************************************************/

static vnd_userial_cb_t vnd_userial;
int hci_comm_bandrate = 1500000;

/*******************************************************************************
**
** Function        userial_to_tcio_baud
**
** Description     helper function converts USERIAL baud rates into TCIO
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_to_tcio_baud(uint8_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == USERIAL_BAUD_115200)
        *baud = B115200;
    else if (cfg_baud == USERIAL_BAUD_4M)
        *baud = B4000000;
    else if (cfg_baud == USERIAL_BAUD_3M)
        *baud = B3000000;
    else if (cfg_baud == USERIAL_BAUD_2M)
        *baud = B2000000;
    else if (cfg_baud == USERIAL_BAUD_1M)
        *baud = B1000000;
    else if (cfg_baud == USERIAL_BAUD_1_5M)
        *baud = B1500000;
    else if (cfg_baud == USERIAL_BAUD_921600)
        *baud = B921600;
    else if (cfg_baud == USERIAL_BAUD_460800)
        *baud = B460800;
    else if (cfg_baud == USERIAL_BAUD_230400)
        *baud = B230400;
    else if (cfg_baud == USERIAL_BAUD_57600)
        *baud = B57600;
    else if (cfg_baud == USERIAL_BAUD_19200)
        *baud = B19200;
    else if (cfg_baud == USERIAL_BAUD_9600)
        *baud = B9600;
    else if (cfg_baud == USERIAL_BAUD_1200)
        *baud = B1200;
    else if (cfg_baud == USERIAL_BAUD_600)
        *baud = B600;
    else {
        ALOGE("userial vendor open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return FALSE;
    }

    return TRUE;
}

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
/*******************************************************************************
**
** Function        userial_ioctl_init_bt_wake
**
** Description     helper function to set the open state of the bt_wake if ioctl
**                  is used. it should not hurt in the rfkill case but it might
**                  be better to compile it out.
**
** Returns         none
**
*******************************************************************************/
void userial_ioctl_init_bt_wake(int fd)
{
    uint32_t bt_wake_state;

    /* assert BT_WAKE through ioctl */
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_GET_ST, &bt_wake_state);
    VNDUSERIALDBG("userial_ioctl_init_bt_wake read back BT_WAKE state=%i", \
               bt_wake_state);
}
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)


/*****************************************************************************
**   Userial Vendor API Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        userial_vendor_init
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_init(void)
{
    vnd_userial.fd = -1;
    snprintf(vnd_userial.port_name, VND_PORT_NAME_MAXLEN, "%s", \
            BLUETOOTH_UART_DEVICE_PORT);
}

/*******************************************************************************
**
** Function        userial_vendor_open
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
int userial_vendor_open(tUSERIAL_CFG *p_cfg)
{
    uint32_t baud;
    uint8_t data_bits;
    uint16_t parity;
    uint8_t stop_bits;

    vnd_userial.fd = -1;

    if (!userial_to_tcio_baud(p_cfg->baud, &baud))
        return -1;

    if (p_cfg->fmt & USERIAL_DATABITS_8)
        data_bits = CS8;
    else if (p_cfg->fmt & USERIAL_DATABITS_7)
        data_bits = CS7;
    else if (p_cfg->fmt & USERIAL_DATABITS_6)
        data_bits = CS6;
    else if (p_cfg->fmt & USERIAL_DATABITS_5)
        data_bits = CS5;
    else {
        ALOGE("userial vendor open: unsupported data bits");
        return -1;
    }

    if (p_cfg->fmt & USERIAL_PARITY_NONE)
        parity = 0;
    else if (p_cfg->fmt & USERIAL_PARITY_EVEN)
        parity = PARENB;
    else if (p_cfg->fmt & USERIAL_PARITY_ODD)
        parity = (PARENB | PARODD);
    else {
        ALOGE("userial vendor open: unsupported parity bit mode");
        return -1;
    }

    if (p_cfg->fmt & USERIAL_STOPBITS_1)
        stop_bits = 0;
    else if (p_cfg->fmt & USERIAL_STOPBITS_2)
        stop_bits = CSTOPB;
    else {
        ALOGE("userial vendor open: unsupported stop bits");
        return -1;
    }

    ALOGI("userial vendor open: opening %s", vnd_userial.port_name);

    if ((vnd_userial.fd = open(vnd_userial.port_name, O_RDWR)) == -1) {
        ALOGE("userial vendor open: unable to open %s", vnd_userial.port_name);
        return -1;
    }

    tcflush(vnd_userial.fd, TCIOFLUSH);

    tcgetattr(vnd_userial.fd, &vnd_userial.termios);
    cfmakeraw(&vnd_userial.termios);

    if (p_cfg->hw_fctrl == USERIAL_HW_FLOW_CTRL_ON) {
        ALOGI("userial vendor open: with HW flowctrl ON");
        vnd_userial.termios.c_cflag |= (CRTSCTS | stop_bits| parity);
    } else {
        ALOGI("userial vendor open: with HW flowctrl OFF");
        vnd_userial.termios.c_cflag &= ~CRTSCTS;
        vnd_userial.termios.c_cflag |= (stop_bits| parity);

    }

    /* set input/output baudrate */
    cfsetospeed(&vnd_userial.termios, baud);
    cfsetispeed(&vnd_userial.termios, baud);
    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);
    tcflush(vnd_userial.fd, TCIOFLUSH);

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    userial_ioctl_init_bt_wake(vnd_userial.fd);
#endif

    ALOGI("device fd = %d open", vnd_userial.fd);

    return vnd_userial.fd;
}

/*******************************************************************************
**
** Function        userial_vendor_close
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_close(void)
{
    int result;

    if (vnd_userial.fd == -1)
        return;

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    /* de-assert bt_wake BEFORE closing port */
    ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
#endif

    ALOGI("device fd = %d close", vnd_userial.fd);
    // flush Tx before close to make sure no chars in buffer
    tcflush(vnd_userial.fd, TCIOFLUSH);
    if ((result = close(vnd_userial.fd)) < 0)
        ALOGE("close(fd:%d) FAILED result:%d", vnd_userial.fd, result);

    vnd_userial.fd = -1;
}

/*******************************************************************************
**
** Function        userial_vendor_set_baud
**
** Description     Set new baud rate
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_set_baud(uint8_t userial_baud)
{
    uint32_t tcio_baud;

    userial_to_tcio_baud(userial_baud, &tcio_baud);

    cfsetospeed(&vnd_userial.termios, tcio_baud);
    cfsetispeed(&vnd_userial.termios, tcio_baud);
    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);

    ALOGI("userial_vendor_set_baud done.");
}

/*******************************************************************************
**
** Function        userial_vendor_ioctl
**
** Description     ioctl inteface
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_ioctl(userial_vendor_ioctl_op_t op, void *p_data)
{
    UNUSED(p_data);

    switch (op) {
#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
        case USERIAL_OP_ASSERT_BT_WAKE:
            VNDUSERIALDBG("## userial_vendor_ioctl: Asserting BT_Wake ##");
            ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
            break;

        case USERIAL_OP_DEASSERT_BT_WAKE:
            VNDUSERIALDBG("## userial_vendor_ioctl: De-asserting BT_Wake ##");
            ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
            break;

        case USERIAL_OP_GET_BT_WAKE_STATE:
            ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_GET_ST, p_data);
            break;
#endif  //  (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

        default:
            break;
    }
}

/*******************************************************************************
**
** Function        userial_set_port
**
** Description     Configure UART port name
**
** Returns         0 : Success
**                 Otherwise : Fail
**
*******************************************************************************/
int userial_set_port(char *p_conf_name, char *p_conf_value, int param)
{
    UNUSED(p_conf_name);
    UNUSED(param);
    strcpy(vnd_userial.port_name, p_conf_value);

    return 0;
}

int userial_set_bandrate(char *p_conf_name, char *p_conf_value, int param)
{
    UNUSED(p_conf_name);
    UNUSED(param);
    hci_comm_bandrate = atoi(p_conf_value);

    return 0;
}

/*******************************************************************************
**
** Function        userial_vendor_set_hw_fctrl
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_set_hw_fctrl(uint8_t hw_fctrl)
{
    if (vnd_userial.fd == -1) {
        ALOGE("vnd_userial.fd is -1");
        return;
    }

    if (hw_fctrl) {
        ALOGI("Set HW FlowControl On");
        vnd_userial.termios.c_cflag |= CRTSCTS;
    } else {
        ALOGI("Set HW FlowControl Off");
        vnd_userial.termios.c_cflag &= ~CRTSCTS;
    }

    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);
    tcflush(vnd_userial.fd, TCIOFLUSH);
}

/*******************************************************************************
**
** Function        line_speed_to_userial_baud
**
** Description     helper function converts line speed number into USERIAL baud
**                 rate symbol
**
** Returns         unit8_t (USERIAL baud symbol)
**
*******************************************************************************/
uint8_t line_speed_to_userial_baud(uint32_t line_speed)
{
    uint8_t baud;

    if (line_speed == 4000000)
        baud = USERIAL_BAUD_4M;
    else if (line_speed == 3000000)
        baud = USERIAL_BAUD_3M;
    else if (line_speed == 2000000)
        baud = USERIAL_BAUD_2M;
    else if (line_speed == 1500000)
        baud = USERIAL_BAUD_1_5M;
    else if (line_speed == 1000000)
        baud = USERIAL_BAUD_1M;
    else if (line_speed == 921600)
        baud = USERIAL_BAUD_921600;
    else if (line_speed == 460800)
        baud = USERIAL_BAUD_460800;
    else if (line_speed == 230400)
        baud = USERIAL_BAUD_230400;
    else if (line_speed == 115200)
        baud = USERIAL_BAUD_115200;
    else if (line_speed == 57600)
        baud = USERIAL_BAUD_57600;
    else if (line_speed == 19200)
        baud = USERIAL_BAUD_19200;
    else if (line_speed == 9600)
        baud = USERIAL_BAUD_9600;
    else if (line_speed == 1200)
        baud = USERIAL_BAUD_1200;
    else if (line_speed == 600)
        baud = USERIAL_BAUD_600;
    else {
        ALOGE("userial vendor: unsupported baud speed %d", line_speed);
        baud = USERIAL_BAUD_115200;
    }

    return baud;
}

void userial_vendor_flush(int selector)
{
    switch (selector) {
        case USERIAL_TCI_FLUSH:
            tcflush(vnd_userial.fd, TCIFLUSH);
            break;
        case USERIAL_TCO_FLUSH:
            tcflush(vnd_userial.fd, TCOFLUSH);
            break;
        default:
            tcflush(vnd_userial.fd, TCIOFLUSH);
            break;
    }
}

uint32_t userial_vendor_read(uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    fd_set set;
    struct timeval tv;
    int rv;

    FD_ZERO(&set);                /* clear the set */
    FD_SET(vnd_userial.fd, &set); /* add our file descriptor to the set */

    /* there was data to read */
    ssize_t r;
    uint8_t *pos = (uint8_t*)buffer;

    while (len > 0) {
        tv.tv_sec  = 0;
        tv.tv_usec = timeout;

        rv = select(vnd_userial.fd + 1, &set, NULL, NULL, &tv);
        if (rv == -1) {
            ALOGE("select error");  /* an error accured */
            break;
        } else if (rv == 0) {
            ALOGE("read timeout");  /* a timeout occured */
            break;
        }

        r = read(vnd_userial.fd, pos, len);
        if (r < 1)
            break;

        len -= r;
        pos += r;
    }

    return pos - buffer;
}

uint32_t userial_vendor_write(const uint8_t *buffer, uint32_t len)
{
    ssize_t r;
    uint8_t *pos = (uint8_t*)buffer;

    while (len > 0) {
        r = write(vnd_userial.fd, pos, len);
        if (r < 1)
            break;

        len -= r;
        pos += r;
    }

    return pos - buffer;
}
