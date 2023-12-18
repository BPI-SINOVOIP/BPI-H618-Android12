#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include "type.h"

#define LOG_TAG "XR_SOCKET"
#include <utils/Log.h>

#define SOCKET_DEBUG       0

typedef long int sem_t;

/* socket control block */
struct socket_cb_t {
    int uart_fd;
    int socket_fd[2];
    int signal_fd[2];
    int epoll_fd;
    int cpoll_fd;
    int event_fd;
    pthread_t thread_socket_id;
    pthread_t thread_uart_id;
    pthread_mutex_t thread_mut;
    bool thread_running;
    sem_t sem;
} socket_cb_t;

static struct socket_cb_t xr_socket;
static char local_name[248] = {'x', 'r', '8', '1', '9', 's', '-', 'b', 'l', 'e'};
static char local_supported_features[8] = {0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x08, 0x00}; // ble, not bredr, ssp

static int handle_controller_to_stack_event(void *arg);
static int handle_stack_to_controller_command(void *arg);
static void *xr_recv_uart_thread(void *arg);
static void *xr_recv_socket_thread(void *arg);
extern bool controller_need_hci_emulate(void);

int xr_socket_open(int uart_fd)
{
    int ret = 0;
    struct epoll_event event;

    xr_socket.uart_fd = uart_fd;
    pthread_mutex_init(&xr_socket.thread_mut, NULL);
    sem_init(&xr_socket.sem, 0, 0);

    if ((ret = socketpair(AF_UNIX, SOCK_STREAM, 0, xr_socket.socket_fd)) < 0) {
        ALOGE("%s, errno : %s", __func__, strerror(errno));
        return ret;
    }

    if ((ret = socketpair(AF_UNIX, SOCK_STREAM, 0, xr_socket.signal_fd)) < 0) {
        ALOGE("%s, errno : %s", __func__, strerror(errno));
        return ret;
    }

    xr_socket.epoll_fd = epoll_create(64);
    if (xr_socket.epoll_fd == -1) {
        ALOGE("%s unable to create epoll instance: %s", __func__, strerror(errno));
        return -1;
    }

    memset(&event, 0, sizeof(event));
    event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    if (epoll_ctl(xr_socket.epoll_fd, EPOLL_CTL_ADD, xr_socket.socket_fd[1], &event) == -1) {
        ALOGE("%s unable to register fd %d to epoll set: %s", __func__, xr_socket.socket_fd[1], strerror(errno));
        close(xr_socket.epoll_fd);
        xr_socket.epoll_fd = -1;
        return -1;
    }

    if (epoll_ctl(xr_socket.epoll_fd, EPOLL_CTL_ADD, xr_socket.signal_fd[1], &event) == -1) {
        ALOGE("%s unable to register signal fd %d to epoll set: %s", __func__, xr_socket.signal_fd[1], strerror(errno));
        close(xr_socket.epoll_fd);
        xr_socket.epoll_fd = -1;
        return -1;
    }

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    xr_socket.thread_running = true;
    if (pthread_create(&xr_socket.thread_socket_id, &thread_attr, xr_recv_socket_thread, NULL) != 0) {
        ALOGE("pthread_create : %s", strerror(errno));
        close(xr_socket.epoll_fd);
        xr_socket.epoll_fd = -1;
        xr_socket.thread_socket_id = -1;
        return -1;
    }

    if (pthread_create(&xr_socket.thread_uart_id, &thread_attr, xr_recv_uart_thread, NULL) != 0) {
        ALOGE("pthread_create : %s", strerror(errno));
        close(xr_socket.epoll_fd);
        xr_socket.thread_running = false;
        pthread_join(xr_socket.thread_socket_id, NULL);
        xr_socket.thread_socket_id = -1;
        return -1;
    }

    xr_socket.cpoll_fd = epoll_create(64);
    assert (xr_socket.cpoll_fd != -1);

    xr_socket.event_fd = eventfd(10, EFD_NONBLOCK);
    assert(xr_socket.event_fd != -1);
    if (xr_socket.event_fd != -1) {
        memset(&event, 0, sizeof(event));
        event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
        if (epoll_ctl(xr_socket.cpoll_fd, EPOLL_CTL_ADD, xr_socket.event_fd, &event) == -1) {
            ALOGE("%s unable to register fd %d to cpoll set: %s", __func__, xr_socket.event_fd, strerror(errno));
            assert(false);
        }

        if (epoll_ctl(xr_socket.cpoll_fd, EPOLL_CTL_ADD, xr_socket.signal_fd[1], &event) == -1) {
            ALOGE("%s unable to register fd %d to cpoll set: %s", __func__, xr_socket.signal_fd[1], strerror(errno));
            assert(false);
        }

    }

    ret = xr_socket.socket_fd[0];
    return ret;
}

void xr_socket_close(void)
{
    int result;

    xr_socket.thread_running = false;
    if ((xr_socket.socket_fd[0] > 0) && (result = close(xr_socket.socket_fd[0])) < 0)
        ALOGE("%s (fd:%d) FAILED result:%d", __func__, xr_socket.socket_fd[0], result);

    if (epoll_ctl(xr_socket.epoll_fd, EPOLL_CTL_DEL, xr_socket.socket_fd[1], NULL) == -1)
        ALOGE("%s unable to unregister fd %d from epoll set: %s", __func__, xr_socket.socket_fd[1], strerror(errno));

    if (epoll_ctl(xr_socket.epoll_fd, EPOLL_CTL_DEL, xr_socket.signal_fd[1], NULL) == -1)
        ALOGE("%s unable to unregister signal fd %d from epoll set: %s", __func__, xr_socket.signal_fd[1], strerror(errno));

    if ((xr_socket.socket_fd[1] > 0) && (result = close(xr_socket.socket_fd[1])) < 0)
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, xr_socket.socket_fd[1], result);

    if (xr_socket.thread_socket_id != -1)
        pthread_join(xr_socket.thread_socket_id, NULL);

    if (xr_socket.epoll_fd > 0)
        close(xr_socket.epoll_fd);

    if ((xr_socket.signal_fd[0] > 0) && (result = close(xr_socket.signal_fd[0])) < 0)
        ALOGE( "%s (signal fd[0]:%d) FAILED result:%d", __func__, xr_socket.signal_fd[0], result);
    if ((xr_socket.signal_fd[1] > 0) && (result = close(xr_socket.signal_fd[1])) < 0)
        ALOGE( "%s (signal fd[1]:%d) FAILED result:%d", __func__, xr_socket.signal_fd[1], result);

    xr_socket.epoll_fd = -1;
    xr_socket.socket_fd[0] = -1;
    xr_socket.socket_fd[1] = -1;
    xr_socket.signal_fd[0] = -1;
    xr_socket.signal_fd[1] = -1;
    pthread_mutex_destroy(&xr_socket.thread_mut);
    sem_destroy(&xr_socket.sem);
    ALOGD("%s exit", __func__);
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
        cnt += sprintf(&dumpbuf[cnt], "|\n");
        ALOGD("%s", &dumpbuf[0]);
        cnt = 0;
        if (i > len - 1)
            break;
        else
            idx += 16;
    }
}

static int event_command_complete(char *buf, int command, char *data, int len)
{
    *(buf + 0) = 0x04;
    *(buf + 1) = 0x0e;
    *(buf + 2) = 0x04 + len;
    *(buf + 3) = 0x01;
    *(buf + 4) = command & 0xff;
    *(buf + 5) = (command >> 8) & 0xff;
    *(buf + 6) = 0x00;
    if (len)
        memcpy(buf + 7, data, len);
    return len + 7;
}

static int event_command_status(char *buf, int command)
{
    *(buf + 0) = 0x04;
    *(buf + 1) = 0x0f;
    *(buf + 2) = 0x04;
    *(buf + 3) = 0x00;
    *(buf + 4) = 0x01;
    *(buf + 5) = command & 0xff;
    *(buf + 6) = (command >> 8) & 0xff;
    return 7;
}

static void *xr_recv_socket_thread(void *arg)
{
    struct epoll_event events[64];
    int i;
    int ret;
    while (xr_socket.thread_running) {
        do {
            ret = epoll_wait(xr_socket.epoll_fd, events, 32, 500);
        } while (xr_socket.thread_running && ret == -1 && errno == EINTR);

        if (ret < 0) {
            ALOGE("%s error in epoll_wait: %s", __func__, strerror(errno));
            continue;
        }

        for (i = 0; i < ret; ++i) {
            if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                handle_stack_to_controller_command(NULL);
            }

            if (events[i].events & EPOLLOUT) {
                ALOGD("socket EPOLLOUT");
            }
        }
    }
    xr_socket.thread_socket_id = -1;
    ALOGD("%s exit", __func__);
    return NULL;
}

static void *xr_recv_uart_thread(void *arg)
{
    struct pollfd pfd[2] = {
            [0] = {.events = POLLIN|POLLHUP|POLLERR|POLLRDHUP, .fd = xr_socket.signal_fd[1]},
            [1] = {.events = POLLIN|POLLHUP|POLLERR|POLLRDHUP, .fd = xr_socket.uart_fd}};
    int ret;

    sem_wait(&xr_socket.sem);
    ALOGD("Stack start...");

    while (xr_socket.thread_running) {
        do {
            ret = poll(pfd, 2, 500);
        } while (ret == -1 && errno == EINTR && xr_socket.thread_running);

        if (ret < 0) {
            ALOGE("%s : error (%d)", __func__, ret);
            continue;
        }

        if (pfd[1].revents & POLLIN) {
            handle_controller_to_stack_event(NULL);
        }

        if (pfd[1].revents & (POLLERR|POLLHUP)) {
            ALOGE("%s poll error", __func__);
            return NULL;
        }
    }
    xr_socket.thread_uart_id = -1;
    ALOGD("%s exit", __func__);
    return NULL;
}

static int handle_stack_to_controller_command(void *arg)
{
    char buf[1024];
    int len;
    int fd;
    int type, command;

    len = read(xr_socket.socket_fd[1], buf, sizeof(buf));

    if (len > 0) {
#if SOCKET_DEBUG
        ALOGD("%s, len: %d", __func__, len);
        hexdump(buf, len);
#endif
        fd = xr_socket.uart_fd;

        if (controller_need_hci_emulate()) {
            type = buf[0];
            command = buf[1] | (buf[2] << 8);
            /* current only hook hci command package */
            if (type == 0x01) {
                switch (command) {
                    case 0x0c24: // Write_Class_of_Device
                    case 0x0c18: // Write_Page_Timeout
                    case 0x0c52: // Write Extended Inquiry Response
                    case 0x0c26: // Write_Voice_Setting
                    case 0x0c3a: // Write_Current_IAC_LAP
                    case 0x0c1e: // Write_Inquiry_Scan_Activity
                    case 0x0c1a: // Write_Scan_Enable
                    case 0x0c0a: // Set_Event_Mask
                    case 0x0c05: // Set_Event_Filter
                        len = event_command_complete(buf, command, NULL, 0);
                        fd  = xr_socket.socket_fd[1];
                        break;
                    case 0x0c13: // Change_Local_Name
                        // save stack bt name to local
                        memcpy(local_name, &buf[4], buf[3]);
                        len = event_command_complete(buf, command, NULL, 0);
                        fd  = xr_socket.socket_fd[1];
                        break;
                    case 0x0c14: // READ_LOCAL_NAME
                        len = event_command_complete(buf, command, local_name, 248);
                        fd  = xr_socket.socket_fd[1];
                        break;
                    case 0x0401: // Inquiry
                    case 0x0402: // Inquiry_Cancel
                    case 0x0c12: // Delete_Stored_Link_Key
                        len = event_command_status(buf, command);
                        fd  = xr_socket.socket_fd[1];
                        break;
                    case 0x1003: // Read_Local_Supported_Features
                        len = event_command_complete(buf, command, local_supported_features, 8);
                        fd  = xr_socket.socket_fd[1];
                        break;
                    default:
                        break;
                }
            }
        }

        if (fd == xr_socket.socket_fd[1])
            pthread_mutex_lock(&xr_socket.thread_mut);

        write(fd, buf, len);

        if (fd == xr_socket.socket_fd[1])
            pthread_mutex_unlock(&xr_socket.thread_mut);
    }
    return 0;
}

static int handle_controller_to_stack_event(void *arg)
{
    char buf[1024];
    int len;
    uint8_t type, evt_code, sub_evt_code;

    len = read(xr_socket.uart_fd, buf, sizeof(buf));
    if (len > 0) {
#if SOCKET_DEBUG
        ALOGD("%s, len: %d", __func__, len);
        hexdump(buf, len);
#endif
        type = buf[0];
        evt_code = buf[1];
        sub_evt_code = buf[3];
        if (type == 0x4 && evt_code == 0xFF && sub_evt_code == 0x57) {
            // printable firmware debug log event
            handle_fw_debug_info(&buf[1]);
        }

        pthread_mutex_lock(&xr_socket.thread_mut);
        write(xr_socket.socket_fd[1], buf, len);
        pthread_mutex_unlock(&xr_socket.thread_mut);
    }
    return 0;
}

int start_stack(void)
{
    sem_post(&xr_socket.sem);
    return 0;
}
