#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include <getopt.h>
#include <termios.h>

#define LOG_TAG "UART-BRIDGE"
#include <utils/Log.h>

#define ARRAY_SIZE(x)           sizeof(x) / sizeof(x[0])

#ifdef DBG_LOGCAT
#define DBG_D                   ALOGD
#define DBG_E                   ALOGE
#else

#define LEVEL_DEF          LEVEL_D
#define DBG_E(fmt, arg...) log_print(LEVEL_E, LOG_TAG, fmt, ##arg)
#define DBG_W(fmt, arg...) log_print(LEVEL_W, LOG_TAG, fmt, ##arg)
#define DBG_I(fmt, arg...) log_print(LEVEL_I, LOG_TAG, fmt, ##arg)
#define DBG_D(fmt, arg...) log_print(LEVEL_D, LOG_TAG, fmt, ##arg)
#define DBG_V(fmt, arg...) log_print(LEVEL_V, LOG_TAG, fmt, ##arg)

enum {
    LEVEL_V,
    LEVEL_D,
    LEVEL_I,
    LEVEL_W,
    LEVEL_E,
};

static int log_print(int level, const char *tag, const char *fmt, ...)
{
    struct tm *lt;
    struct timeval tv;
    time_t t;
    char banner;
    va_list arg;

    if (level < LEVEL_DEF)
        return 0;

    gettimeofday(&tv, NULL);
    t = (time_t)tv.tv_sec;
    lt = localtime(&t);

    va_start(arg, fmt);

    switch (level) {
        case LEVEL_E:
            banner = 'E';
            break;
        case LEVEL_W:
            banner = 'W';
            break;
        case LEVEL_I:
            banner = 'I';
            break;
        case LEVEL_D:
            banner = 'D';
            break;
        case LEVEL_V:
            banner = 'V';
            break;
        default:
            banner = 'U';
            break;
    }

    fprintf(stdout, "%02d-%02d %02d:%02d:%02d.%03u %5d %5d %c %-10s: ",
            lt->tm_mon,
            lt->tm_mday,
            lt->tm_hour,
            lt->tm_min,
            lt->tm_sec,
            (unsigned)tv.tv_usec / 1000,
            getpid(),
            getppid(),
            banner,
            tag);

    vfprintf(stdout, fmt, arg);
    fprintf(stdout, "\n");

    va_end(arg);

    return 0;
}

#endif

/**** Data Format ****/
/* Stop Bits */
#define USERIAL_STOPBITS_1      (1<<0)
#define USERIAL_STOPBITS_1_5    (1<<1)
#define USERIAL_STOPBITS_2      (1<<2)

/* Parity Bits */
#define USERIAL_PARITY_NONE     (1<<3)
#define USERIAL_PARITY_EVEN     (1<<4)
#define USERIAL_PARITY_ODD      (1<<5)

/* Data Bits */
#define USERIAL_DATABITS_5      (1<<6)
#define USERIAL_DATABITS_6      (1<<7)
#define USERIAL_DATABITS_7      (1<<8)
#define USERIAL_DATABITS_8      (1<<9)

/* HW flow ctrl */
#define USERIAL_HW_FC           (1<<10)

/* Structure used to configure serial port during open */
struct serial_cfg_t {
    uint16_t fmt;       /* Data format */
    uint32_t  baud;      /* Baud rate */
    int fd;
    struct termios termios;
    const char *port_name;
};

/* control block */
struct bridge_t {
    struct serial_cfg_t bt_uart;
    struct serial_cfg_t ap_uart;
    int epoll_ap_fd;
    int epoll_bt_fd;
    pthread_t thread_ap_uart_id;
    pthread_t thread_bt_uart_id;
    pthread_mutex_t thread_mut;
    bool thread_running;
    bool debug;
};

static struct bridge_t bridge = {
    .bt_uart = {
        .fmt  = USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1 | USERIAL_HW_FC,
        .baud = 1500000,
        .port_name = "/dev/ttyAS1",
    },

    .ap_uart = {
        .fmt  = USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1,
        .baud = 115200,
        .port_name = "/dev/ttyGS0",
    },

    .debug = false,
};

struct option_t {
    const struct option long_options;
    const char *help;
};

static const struct option_t option_list[] = {
    {{"bt-dev-path",  1, NULL, 'd'}, "bt uart device path, default /dev/ttyAS1"},
    {{"ap-dev-path",  1, NULL, 'D'}, "ap uart device path, default /dev/ttyGS0"},
    {{"bt-flow-ctrl", 1, NULL, 'f'}, "enable bt uart flow control, default enable"},
    {{"ap-flow-ctrl", 1, NULL, 'F'}, "enable ap uart flow control, default disable"},
    {{"bt-baud-rate", 1, NULL, 'b'}, "bt uart baud rate, default 1500000"},
    {{"ap-baud-rate", 1, NULL, 'B'}, "ap uart baud rate, default 115200"},
    {{"bt-parity",    1, NULL, 'p'}, "bt uart parity, default none"},
    {{"ap-parity",    1, NULL, 'P'}, "ap uart parity, default none"},
    {{"dump-message", 0, NULL, 'm'}, "show data dump debug message, default off"},
};

static int handle_ap_to_bt_data(void *arg);
static int handle_bt_to_ap_data(void *arg);
static void *bt_uart_recv_thread(void *arg);
static void *ap_uart_recv_thread(void *arg);

int uart_brigde_open(void)
{
    int ret = -1;
    struct epoll_event event;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    bridge.epoll_ap_fd = epoll_create(64);
    if (bridge.epoll_ap_fd == -1) {
        DBG_E("%s unable to create epoll instance: %s", __func__, strerror(errno));
        return ret;
    }

    memset(&event, 0, sizeof(event));
    event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    if (epoll_ctl(bridge.epoll_ap_fd, EPOLL_CTL_ADD, bridge.ap_uart.fd, &event) == -1) {
        DBG_E("%s unable to register fd %d to epoll set: %s", __func__, bridge.ap_uart.fd, strerror(errno));
        goto err_ap_epoll;
    }

    bridge.epoll_bt_fd = epoll_create(64);
    if (bridge.epoll_bt_fd == -1) {
        DBG_E("%s unable to create epoll instance: %s", __func__, strerror(errno));
        goto err_ap_epoll;
    }

    memset(&event, 0, sizeof(event));
    event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    if (epoll_ctl(bridge.epoll_bt_fd, EPOLL_CTL_ADD, bridge.bt_uart.fd, &event) == -1) {
        DBG_E("%s unable to register fd %d to epoll set: %s", __func__, bridge.bt_uart.fd, strerror(errno));
        goto err_bt_epoll;
    }

    bridge.thread_running = true;
    if (pthread_create(&bridge.thread_ap_uart_id, &thread_attr, ap_uart_recv_thread, NULL) != 0) {
        DBG_E("pthread_create : %s", strerror(errno));
        goto err_ap_thread;
    }

    if (pthread_create(&bridge.thread_bt_uart_id, &thread_attr, bt_uart_recv_thread, NULL) != 0) {
        DBG_E("pthread_create : %s", strerror(errno));
        goto err_bt_thread;
    }

    DBG_D("%s done", __func__);
    return 0;

err_bt_thread:
    bridge.thread_bt_uart_id = -1;
    bridge.thread_running = false;
    pthread_join(bridge.thread_ap_uart_id, NULL);

err_ap_thread:
    bridge.thread_ap_uart_id = -1;

err_bt_epoll:
    close(bridge.epoll_bt_fd);
    bridge.epoll_bt_fd = -1;

err_ap_epoll:
    close(bridge.epoll_ap_fd);
    bridge.epoll_ap_fd = -1;

    return ret;
}

void uart_brigde_close(void)
{
    int result;

    bridge.thread_running = false;
    if (epoll_ctl(bridge.epoll_ap_fd, EPOLL_CTL_DEL, bridge.ap_uart.fd, NULL) == -1)
        DBG_E("%s unable to unregister fd %d from epoll set: %s", __func__, bridge.ap_uart.fd, strerror(errno));

    if ((bridge.ap_uart.fd > 0) && (result = close(bridge.ap_uart.fd)) < 0)
        DBG_E( "%s (fd:%d) FAILED result:%d", __func__, bridge.ap_uart.fd, result);

    if (epoll_ctl(bridge.epoll_bt_fd, EPOLL_CTL_DEL, bridge.bt_uart.fd, NULL) == -1)
        DBG_E("%s unable to unregister fd %d from epoll set: %s", __func__, bridge.bt_uart.fd, strerror(errno));

    if ((bridge.bt_uart.fd > 0) && (result = close(bridge.bt_uart.fd)) < 0)
        DBG_E( "%s (fd:%d) FAILED result:%d", __func__, bridge.bt_uart.fd, result);

    if (bridge.thread_ap_uart_id != -1)
        pthread_join(bridge.thread_ap_uart_id, NULL);

    if (bridge.epoll_ap_fd > 0)
        close(bridge.epoll_ap_fd);

    if (bridge.thread_bt_uart_id != -1)
        pthread_join(bridge.thread_bt_uart_id, NULL);

    if (bridge.epoll_bt_fd > 0)
        close(bridge.epoll_bt_fd);

    bridge.epoll_bt_fd = -1;
    bridge.epoll_ap_fd = -1;
    bridge.ap_uart.fd  = -1;
    bridge.bt_uart.fd  = -1;
    pthread_mutex_destroy(&bridge.thread_mut);
    DBG_D("%s done", __func__);
}

static void hexdump(char *buf, int len)
{
    int i, cnt = 0, idx = 0;
    char dumpbuf[128];
    if (len <= 0)
        return;

    while (1) {
        cnt += sprintf(&dumpbuf[cnt], "%04x  ", idx / 16);
        for (i = idx; i < idx + 16; i++) {
            if (i > len - 1)
                cnt += sprintf(&dumpbuf[cnt], "%s ", "  ");
            else
                cnt += sprintf(&dumpbuf[cnt], "%02x ", (unsigned char)buf[i]);
            if (i % 16 == 7  || i % 16 == 15)
                cnt += sprintf(&dumpbuf[cnt], " ");
        }
        cnt += sprintf(&dumpbuf[cnt], "|");
        for (i = idx; i < idx + 16; i++) {
            if (i > len - 1) {
                cnt += sprintf(&dumpbuf[cnt], "%c", '.');
            } else {
                if (buf[i] < 0x20 || buf[i] >= 0x7f)
                    cnt += sprintf(&dumpbuf[cnt], "%c", '.');
                else
                    cnt += sprintf(&dumpbuf[cnt], "%c", buf[i]);
            }
        }
        cnt += sprintf(&dumpbuf[cnt], "|");
        DBG_D("%s", &dumpbuf[0]);
        cnt = 0;
        if (i > len - 1)
            break;
        else
            idx += 16;
    }
}

static void *bt_uart_recv_thread(void *arg)
{
    struct epoll_event events[64];
    int i, len;
    (void)arg;

    while (bridge.thread_running) {
        do {
            len = epoll_wait(bridge.epoll_bt_fd, events, 32, 500);
        } while (bridge.thread_running && len == -1 && errno == EINTR);

        if (len < 0) {
            DBG_E("%s error in epoll_wait: %s", __func__, strerror(errno));
            continue;
        }

        for (i = 0; i < len; ++i) {
            if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                handle_bt_to_ap_data(NULL);
            }

            if (events[i].events & EPOLLOUT) {
                DBG_D("bt uart EPOLLOUT");
            }
        }
    }
    bridge.thread_ap_uart_id = -1;
    DBG_D("%s exit", __func__);
    return NULL;
}

static void *ap_uart_recv_thread(void *arg)
{
    struct epoll_event events[64];
    int i, len;
    (void)arg;

    while (bridge.thread_running) {
        do {
            len = epoll_wait(bridge.epoll_ap_fd, events, 32, 500);
        } while (bridge.thread_running && len == -1 && errno == EINTR);

        if (len < 0) {
            DBG_E("%s error in epoll_wait: %s", __func__, strerror(errno));
            continue;
        }

        for (i = 0; i < len; ++i) {
            if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                handle_ap_to_bt_data(NULL);
            }

            if (events[i].events & EPOLLOUT) {
                DBG_D("ap uart EPOLLOUT");
            }
        }
    }
    bridge.thread_bt_uart_id = -1;
    DBG_D("%s exit", __func__);
    return NULL;
}

static int handle_ap_to_bt_data(void *arg)
{
    char buf[1024];
    int len, i;
    (void)arg;

    while ((len = read(bridge.ap_uart.fd, buf, sizeof(buf))) > 0) {
        if (bridge.debug) {
            DBG_D("%s, len: %d", __func__, len);
            hexdump(buf, len);
        }

        i = 0;
        while (i < len) {
            i += write(bridge.bt_uart.fd, &buf[i], len - i);
        }
    }
    return 0;
}

static int handle_bt_to_ap_data(void *arg)
{
    char buf[1024];
    int len, i;
    (void)arg;

    while ((len = read(bridge.bt_uart.fd, buf, sizeof(buf))) > 0) {
        if (bridge.debug) {
            DBG_D("%s, len: %d", __func__, len);
            hexdump(buf, len);
        }

        i = 0;
        while (i < len) {
            i += write(bridge.ap_uart.fd, &buf[i], len - i);
        }
    }
    return 0;
}

static int userial_to_tcio_baud(uint32_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == 115200)
        *baud = B115200;
    else if (cfg_baud == 4000000)
        *baud = B4000000;
    else if (cfg_baud == 3500000)
        *baud = B3500000;
    else if (cfg_baud == 3000000)
        *baud = B3000000;
    else if (cfg_baud == 2500000)
        *baud = B2500000;
    else if (cfg_baud == 2000000)
        *baud = B2000000;
    else if (cfg_baud == 1500000)
        *baud = B1500000;
    else if (cfg_baud == 1000000)
        *baud = B1000000;
    else if (cfg_baud == 921600)
        *baud = B921600;
    else if (cfg_baud == 460800)
        *baud = B460800;
    else if (cfg_baud == 230400)
        *baud = B230400;
    else if (cfg_baud == 57600)
        *baud = B57600;
    else if (cfg_baud == 19200)
        *baud = B19200;
    else if (cfg_baud == 9600)
        *baud = B9600;
    else if (cfg_baud == 1200)
        *baud = B1200;
    else if (cfg_baud == 600)
        *baud = B600;
    else {
        ALOGE( "userial vendor open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return -1;
    }

    return 0;
}

static int userial_vendor_open(struct serial_cfg_t *p_cfg)
{
    uint32_t baud;
    uint8_t data_bits;
    uint16_t parity;
    uint8_t stop_bits;

    if (userial_to_tcio_baud(p_cfg->baud, &baud)) {
        return -1;
    }

    if (p_cfg->fmt & USERIAL_DATABITS_8)
        data_bits = CS8;
    else if (p_cfg->fmt & USERIAL_DATABITS_7)
        data_bits = CS7;
    else if (p_cfg->fmt & USERIAL_DATABITS_6)
        data_bits = CS6;
    else if (p_cfg->fmt & USERIAL_DATABITS_5)
        data_bits = CS5;
    else {
        DBG_E("userial vendor open: unsupported data bits");
        return -1;
    }

    if (p_cfg->fmt & USERIAL_PARITY_NONE)
        parity = 0;
    else if (p_cfg->fmt & USERIAL_PARITY_EVEN)
        parity = PARENB;
    else if (p_cfg->fmt & USERIAL_PARITY_ODD)
        parity = (PARENB | PARODD);
    else {
        DBG_E("userial vendor open: unsupported parity bit mode");
        return -1;
    }

    if (p_cfg->fmt & USERIAL_STOPBITS_1)
        stop_bits = 0;
    else if (p_cfg->fmt & USERIAL_STOPBITS_2)
        stop_bits = CSTOPB;
    else {
        DBG_E("userial vendor open: unsupported stop bits");
        return -1;
    }

    ALOGI("userial vendor open: opening %s", p_cfg->port_name);

    if ((p_cfg->fd = open(p_cfg->port_name, O_RDWR | O_NONBLOCK)) == -1) {
        DBG_E("userial vendor open: unable to open %s", p_cfg->port_name);
        return -1;
    }
    tcflush(p_cfg->fd, TCIOFLUSH);

    tcgetattr(p_cfg->fd, &p_cfg->termios);
    cfmakeraw(&p_cfg->termios);
    p_cfg->termios.c_cflag |= stop_bits;
    if (p_cfg->fmt & USERIAL_HW_FC)
        p_cfg->termios.c_cflag |= CRTSCTS;
    else
        p_cfg->termios.c_cflag &= ~CRTSCTS;

    tcsetattr(p_cfg->fd, TCSANOW, &p_cfg->termios);
    tcflush(p_cfg->fd, TCIOFLUSH);

    tcsetattr(p_cfg->fd, TCSANOW, &p_cfg->termios);
    tcflush(p_cfg->fd, TCIOFLUSH);
    tcflush(p_cfg->fd, TCIOFLUSH);

    /* set input/output baudrate */
    cfsetospeed(&p_cfg->termios, baud);
    cfsetispeed(&p_cfg->termios, baud);
    tcsetattr(p_cfg->fd, TCSANOW, &p_cfg->termios);

    ALOGI("device fd = %d open", p_cfg->fd);

    return p_cfg->fd;
}

static void handle_termination(int signo)
{
    DBG_D("Reviced signal [%d], cleanup...", signo);
    uart_brigde_close();
    DBG_D("Termination~");
    _exit(0);
}

int show_help(const char *name)
{
    int i;
    DBG_D("Usage: %s [d|D|f|F|b|B|p|P]", name);
    DBG_D("");

    for (i = 0; i < ARRAY_SIZE(option_list); i++)
        DBG_D("\t-%c, --%-10s %s",
                    option_list[i].long_options.val,
                    option_list[i].long_options.name,
                    option_list[i].help);

    DBG_D("");
    return 0;
}

int main(int argc __unused, char **argv __unused)
{
    int ret, opt, i;
    struct option long_options[ARRAY_SIZE(option_list)];

    for (i = 0; i < ARRAY_SIZE(option_list); i++) {
        long_options[i] = option_list[i].long_options;
    }

    while ((opt = getopt_long(argc, argv, "d:D:f:F:b:B:p:P:mh", long_options, NULL)) != -1) {
        switch(opt) {
            case 'd':
                bridge.bt_uart.port_name = optarg;
                break;
            case 'D':
                bridge.ap_uart.port_name = optarg;
                break;
            case 'f':
                if (strtoul(optarg, 0, 0))
                    bridge.bt_uart.fmt |= USERIAL_HW_FC;
                else
                    bridge.bt_uart.fmt &= ~USERIAL_HW_FC;
                break;
            case 'F':
                if (strtoul(optarg, 0, 0))
                    bridge.ap_uart.fmt |= USERIAL_HW_FC;
                else
                    bridge.ap_uart.fmt &= ~USERIAL_HW_FC;
                break;
            case 'b':
                bridge.bt_uart.baud = strtoul(optarg, 0, 0);
                break;
            case 'B':
                bridge.ap_uart.baud = strtoul(optarg, 0, 0);
                break;
            case 'p':
                bridge.bt_uart.fmt &= ~(USERIAL_PARITY_NONE | USERIAL_PARITY_ODD | USERIAL_PARITY_EVEN);
                if (strncmp(optarg, "even", 4)) {
                    bridge.bt_uart.fmt |= USERIAL_PARITY_EVEN;
                } else if (strncmp(optarg, "odd", 4)) {
                    bridge.bt_uart.fmt |= USERIAL_PARITY_ODD;
                } else {
                    bridge.bt_uart.fmt |= USERIAL_PARITY_NONE;
                }
                break;
            case 'P':
                bridge.ap_uart.fmt &= ~(USERIAL_PARITY_NONE | USERIAL_PARITY_ODD | USERIAL_PARITY_EVEN);
                if (strncmp(optarg, "even", 4)) {
                    bridge.ap_uart.fmt |= USERIAL_PARITY_EVEN;
                } else if (strncmp(optarg, "odd", 4)) {
                    bridge.ap_uart.fmt |= USERIAL_PARITY_ODD;
                } else {
                    bridge.ap_uart.fmt |= USERIAL_PARITY_NONE;
                }
                break;
            case 'm':
                bridge.debug = true;
                break;
            default:
                show_help(argv[0]);
                return 0;
        }
    }

    if (userial_vendor_open(&bridge.bt_uart) < 0) {
        DBG_E("open bt uart err");
        goto err_bt_uart;
    }

    if (userial_vendor_open(&bridge.ap_uart) < 0) {
        DBG_E("open ap uart err");
        goto err_ap_uart;
    }

    ret = uart_brigde_open();
    if (ret < 0) {
        DBG_E("open brigde err");
        goto err_brigde_open;
    }

    signal(SIGINT, handle_termination);

    while (1) {
        sleep(UINT32_MAX);
    }
    return 0;

err_brigde_open:
    close(bridge.ap_uart.fd);
err_ap_uart:
    close(bridge.bt_uart.fd);
err_bt_uart:
    return -1;
}
