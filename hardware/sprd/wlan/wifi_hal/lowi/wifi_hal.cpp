#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>
#include <errno.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>

#include <dirent.h>
#include <net/if.h>

#include "sync.h"

#define LOG_TAG  "WifiLOWIHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include "rtt.h"
/*
 BUGBUG: normally, libnl allocates ports for all connections it makes; but
 being a static library, it doesn't really know how many other netlink connections
 are made by the same process, if connections come from different shared libraries.
 These port assignments exist to solve that problem - temporarily. We need to fix
 libnl to try and allocate ports across the entire process.
 */

#define WIFI_HAL_CMD_SOCK_PORT       646
#define WIFI_HAL_EVENT_SOCK_PORT     647

static int internal_no_seq_check(nl_msg *msg, void *arg);
static int internal_valid_message_handler(nl_msg *msg, void *arg);
static int wifi_get_multicast_id(lowi_wifi_handle handle, const char *name, const char *group);
static int wifi_add_membership(lowi_wifi_handle handle, const char *group);
static wifi_error lowi_wifi_init_interfaces(lowi_wifi_handle handle);

/* Initialize/Cleanup */

void wifi_socket_set_local_port(struct nl_sock *sock, uint32_t port)
{
    uint32_t pid = getpid() & 0x3FFFFF;
    nl_socket_set_local_port(sock, pid + (port << 22));
}

static nl_sock * wifi_create_nl_socket(int port)
{
    // ALOGI("Creating socket");
    struct nl_sock *sock = nl_socket_alloc();
    if (sock == NULL) {
        ALOGE("Could not create handle");
        return NULL;
    }

    wifi_socket_set_local_port(sock, port);

    //struct sockaddr *addr = NULL;
    // ALOGI("sizeof(sockaddr) = %d, sizeof(sockaddr_nl) = %d", sizeof(*addr), sizeof(*addr_nl));

    // ALOGI("Connecting socket");
    if (nl_connect(sock, NETLINK_GENERIC)) {
        ALOGE("Could not connect handle");
        nl_socket_free(sock);
        return NULL;
    }

    // ALOGI("Making socket nonblocking");
    /*
    if (nl_socket_set_nonblocking(sock)) {
        ALOGE("Could make socket non-blocking");
        nl_socket_free(sock);
        return NULL;
    }
    */

    return sock;
}


wifi_error lowi_wifi_initialize(lowi_wifi_handle *handle)
{
    srand(getpid());

    ALOGI("Initializing wifi");
    hal_info *info = (hal_info *)malloc(sizeof(hal_info));
    if (info == NULL) {
        ALOGE("Could not allocate hal_info");
        return WIFI_ERROR_UNKNOWN;
    }

    memset(info, 0, sizeof(*info));

    ALOGI("Creating socket");
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, info->cleanup_socks) == -1) {
        ALOGE("Could not create cleanup sockets");
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_sock *cmd_sock = wifi_create_nl_socket(WIFI_HAL_CMD_SOCK_PORT);
    if (cmd_sock == NULL) {
        ALOGE("Could not create handle");
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_sock *event_sock = wifi_create_nl_socket(WIFI_HAL_EVENT_SOCK_PORT);
    if (event_sock == NULL) {
        ALOGE("Could not create handle");
        nl_socket_free(cmd_sock);
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_cb *cb = nl_socket_get_cb(event_sock);
    if (cb == NULL) {
        ALOGE("Could not create handle");
        nl_socket_free(cmd_sock);
        nl_socket_free(event_sock);
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    // ALOGI("cb->refcnt = %d", cb->cb_refcnt);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, internal_no_seq_check, info);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, internal_valid_message_handler, info);
    nl_cb_put(cb);

    info->cmd_sock = cmd_sock;
    info->event_sock = event_sock;
    info->clean_up = false;
    info->in_event_loop = false;

    info->event_cb = (cb_info *)malloc(sizeof(cb_info) * DEFAULT_EVENT_CB_SIZE);
    info->alloc_event_cb = DEFAULT_EVENT_CB_SIZE;
    info->num_event_cb = 0;

    info->cmd = (cmd_info *)malloc(sizeof(cmd_info) * DEFAULT_CMD_SIZE);
    info->alloc_cmd = DEFAULT_CMD_SIZE;
    info->num_cmd = 0;

    info->nl80211_family_id = genl_ctrl_resolve(cmd_sock, "nl80211");
    if (info->nl80211_family_id < 0) {
        ALOGE("Could not resolve nl80211 familty id");
        nl_socket_free(cmd_sock);
        nl_socket_free(event_sock);
        free(info);
        return WIFI_ERROR_UNKNOWN;
    }

    pthread_mutex_init(&info->cb_lock, NULL);

    *handle = (lowi_wifi_handle) info;

    if (lowi_wifi_init_interfaces(*handle) != WIFI_SUCCESS) {
        ALOGE("No wifi interface found");
        nl_socket_free(cmd_sock);
        nl_socket_free(event_sock);
        pthread_mutex_destroy(&info->cb_lock);
        free(info);
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    if ((wifi_add_membership(*handle, "scan") < 0) ||
        (wifi_add_membership(*handle, "mlme")  < 0) ||
        (wifi_add_membership(*handle, "regulatory") < 0) ||
        (wifi_add_membership(*handle, "vendor") < 0)) {
        ALOGE("Add membership failed");
        nl_socket_free(cmd_sock);
        nl_socket_free(event_sock);
        pthread_mutex_destroy(&info->cb_lock);
        free(info);
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    // ALOGI("Found %d interfaces", info->num_interfaces);

    ALOGI("Initialized Wifi HAL Successfully; vendor cmd = %d", NL80211_CMD_VENDOR);
    return WIFI_SUCCESS;
}

wifi_error lowi_wifi_get_ifaces(lowi_wifi_handle handle, int *num, lowi_wifi_interface_handle **interfaces)
{
    hal_info *info = (hal_info *)handle;

    *interfaces = (lowi_wifi_interface_handle *)info->interfaces;
    *num = info->num_interfaces;

    return WIFI_SUCCESS;
}

wifi_error lowi_wifi_get_iface_name(lowi_wifi_interface_handle handle, char *name, size_t size)
{
    interface_info *info = (interface_info *)handle;
    strcpy(name, info->name);
    return WIFI_SUCCESS;
}


static int wifi_add_membership(lowi_wifi_handle handle, const char *group)
{
    hal_info *info = getHalInfo(handle);

    int id = wifi_get_multicast_id(handle, "nl80211", group);
    if (id < 0) {
        ALOGE("Could not find group %s", group);
        return id;
    }

    int ret = nl_socket_add_membership(info->event_sock, id);
    if (ret < 0) {
        ALOGE("Could not add membership to group %s", group);
    }

    // ALOGI("Successfully added membership for group %s", group);
    return ret;
}

static void internal_cleaned_up_handler(lowi_wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    wifi_cleaned_up_handler cleaned_up_handler = info->cleaned_up_handler;

    if (info->cmd_sock != 0) {
        close(info->cleanup_socks[0]);
        close(info->cleanup_socks[1]);
        nl_socket_free(info->cmd_sock);
        nl_socket_free(info->event_sock);
        info->cmd_sock = NULL;
        info->event_sock = NULL;
    }

    (*cleaned_up_handler)((wifi_handle)handle);
    pthread_mutex_destroy(&info->cb_lock);
    free(info);

    ALOGI("Internal cleanup completed");
}

void lowi_wifi_cleanup(lowi_wifi_handle handle, lowi_wifi_cleaned_up_handler handler)
{
    hal_info *info = getHalInfo(handle);
    char buf[64];

    info->cleaned_up_handler = (wifi_cleaned_up_handler)handler;
    if (TEMP_FAILURE_RETRY(write(info->cleanup_socks[0], "Exit", 4)) < 1) {
        // As a fallback set the cleanup flag to TRUE
        ALOGE("could not write to the cleanup socket");
    } else {
        // Listen to the response
        // Hopefully we dont get errors or get hung up
        // Not much can be done in that case, but assume that
        // it has rx'ed the Exit message to exit the thread.
        // As a fallback set the cleanup flag to TRUE
        memset(buf, 0, sizeof(buf));
        ssize_t result = TEMP_FAILURE_RETRY(read(info->cleanup_socks[0], buf, sizeof(buf)));
        ALOGE("%s: Read after POLL returned %zd, error no = %d (%s)", __FUNCTION__,
               result, errno, strerror(errno));
        if (strncmp(buf, "Done", 4) == 0) {
            ALOGE("Event processing terminated");
        } else {
            ALOGD("Rx'ed %s", buf);
        }
    }
    info->clean_up = true;
    pthread_mutex_lock(&info->cb_lock);

    int bad_commands = 0;

    for (int i = 0; i < info->num_event_cb; i++) {
        cb_info *cbi = &(info->event_cb[i]);
        WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
        ALOGI("Command left in event_cb %p:%s", cmd, (cmd ? cmd->getType(): ""));
    }

    while (info->num_cmd > bad_commands) {
        int num_cmd = info->num_cmd;
        cmd_info *cmdi = &(info->cmd[bad_commands]);
        WifiCommand *cmd = cmdi->cmd;
        if (cmd != NULL) {
            ALOGI("Cancelling command %p:%s", cmd, cmd->getType());
            pthread_mutex_unlock(&info->cb_lock);
            cmd->cancel();
            pthread_mutex_lock(&info->cb_lock);
            if (num_cmd == info->num_cmd) {
                ALOGI("Cancelling command %p:%s did not work", cmd, (cmd ? cmd->getType(): ""));
                bad_commands++;
            }
            /* release reference added when command is saved */
            cmd->releaseRef();
        }
    }

    for (int i = 0; i < info->num_event_cb; i++) {
        cb_info *cbi = &(info->event_cb[i]);
        WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
        ALOGE("Leaked command %p", cmd);
    }
    pthread_mutex_unlock(&info->cb_lock);
    internal_cleaned_up_handler(handle);
}

static int internal_pollin_handler(lowi_wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    struct nl_cb *cb = nl_socket_get_cb(info->event_sock);
    int res = nl_recvmsgs(info->event_sock, cb);
    // ALOGD("nl_recvmsgs returned %d", res);
    nl_cb_put(cb);
    return res;
}


///////////////////////////////////////////////////////////////////////////////////////

static int internal_no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

static int internal_valid_message_handler(nl_msg *msg, void *arg)
{
    // ALOGI("got an event");

    lowi_wifi_handle handle = (lowi_wifi_handle)arg;
    hal_info *info = getHalInfo(handle);

    WifiEvent event(msg);
    int res = event.parse();
    if (res < 0) {
        ALOGE("Failed to parse event: %d", res);
        return NL_SKIP;
    }

    int cmd = event.get_cmd();
    uint32_t vendor_id = 0;
    int subcmd = 0;

    if (cmd == NL80211_CMD_VENDOR) {
        vendor_id = event.get_u32(NL80211_ATTR_VENDOR_ID);
        subcmd = event.get_u32(NL80211_ATTR_VENDOR_SUBCMD);
        ALOGV("event received %s, vendor_id = 0x%0x, subcmd = 0x%0x",
                event.get_cmdString(), vendor_id, subcmd);
    } else {
        // ALOGV("event received %s", event.get_cmdString());
    }

    // ALOGV("event received %s, vendor_id = 0x%0x", event.get_cmdString(), vendor_id);
    // event.log();

    //bool dispatched = false;

    pthread_mutex_lock(&info->cb_lock);

    for (int i = 0; i < info->num_event_cb; i++) {
        if (cmd == info->event_cb[i].nl_cmd) {
            if (cmd == NL80211_CMD_VENDOR
                && ((vendor_id != info->event_cb[i].vendor_id)
                || (subcmd != info->event_cb[i].vendor_subcmd)))
            {
                /* event for a different vendor, ignore it */
                continue;
            }

            cb_info *cbi = &(info->event_cb[i]);
            nl_recvmsg_msg_cb_t cb_func = cbi->cb_func;
            void *cb_arg = cbi->cb_arg;
            WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
            if (cmd != NULL) {
                cmd->addRef();
            }
            pthread_mutex_unlock(&info->cb_lock);
            if (cb_func)
                (*cb_func)(msg, cb_arg);
            if (cmd != NULL) {
                cmd->releaseRef();
            }

            return NL_OK;
        }
    }

    pthread_mutex_unlock(&info->cb_lock);
    return NL_OK;
}


class GetMulticastIdCommand : public WifiCommand
{
private:
    const char *mName;
    const char *mGroup;
    int   mId;
public:
    GetMulticastIdCommand(lowi_wifi_handle handle, const char *name, const char *group)
        : WifiCommand("GetMulticastIdCommand", handle, 0)
    {
        mName = name;
        mGroup = group;
        mId = -1;
    }

    int getId() {
        return mId;
    }

    virtual int create() {
        int nlctrlFamily = genl_ctrl_resolve(mInfo->cmd_sock, "nlctrl");
        // ALOGI("ctrl family = %d", nlctrlFamily);
        int ret = mMsg.create(nlctrlFamily, CTRL_CMD_GETFAMILY, 0, 0);
        if (ret < 0) {
            return ret;
        }
        ret = mMsg.put_string(CTRL_ATTR_FAMILY_NAME, mName);
        return ret;
    }

    virtual int handleResponse(WifiEvent& reply) {

        // ALOGI("handling reponse in %s", __func__);

        struct nlattr **tb = reply.attributes();
        //struct genlmsghdr *gnlh = reply.header();
        struct nlattr *mcgrp = NULL;
        int i;

        if (!tb[CTRL_ATTR_MCAST_GROUPS]) {
            ALOGI("No multicast groups found");
            return NL_SKIP;
        } else {
            // ALOGI("Multicast groups attr size = %d", nla_len(tb[CTRL_ATTR_MCAST_GROUPS]));
        }

        for_each_attr(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i) {

            // ALOGI("Processing group");
            struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
            nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (nlattr *)nla_data(mcgrp),
                nla_len(mcgrp), NULL);
            if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] || !tb2[CTRL_ATTR_MCAST_GRP_ID]) {
                continue;
            }

            char *grpName = (char *)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]);
            int grpNameLen = nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME]);

            // ALOGI("Found group name %s", grpName);

            if (strncmp(grpName, mGroup, grpNameLen) != 0)
                continue;

            mId = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
            break;
        }

        return NL_SKIP;
    }

};

static int wifi_get_multicast_id(lowi_wifi_handle handle, const char *name, const char *group)
{
    GetMulticastIdCommand cmd(handle, name, group);
    int res = cmd.requestResponse();
    if (res < 0)
        return res;
    else
        return cmd.getId();
}

/////////////////////////////////////////////////////////////////////////

static bool is_wifi_interface(const char *name)
{
    if (strncmp(name, "wlan", 4) != 0 && strncmp(name, "p2p", 3) != 0) {
        /* not a wifi interface; ignore it */
        return false;
    } else {
        return true;
    }
}

static int get_interface(const char *name, interface_info *info)
{
    strcpy(info->name, name);
    info->id = if_nametoindex(name);
    // ALOGI("found an interface : %s, id = %d", name, info->id);
    return WIFI_SUCCESS;
}

wifi_error lowi_wifi_init_interfaces(lowi_wifi_handle handle)
{
    hal_info *info = (hal_info *)handle;

    struct dirent *de;

    DIR *d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    int n = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name) ) {
            n++;
        }
    }

    closedir(d);

    if (n == 0)
        return WIFI_ERROR_NOT_AVAILABLE;

    d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    info->interfaces = (interface_info **)malloc(sizeof(interface_info *) * n);

    int i = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name)) {
            interface_info *ifinfo = (interface_info *)malloc(sizeof(interface_info));
            if (get_interface(de->d_name, ifinfo) != WIFI_SUCCESS) {
                free(ifinfo);
                continue;
            }
            ifinfo->handle = handle;
            info->interfaces[i] = ifinfo;
            i++;
        }
    }

    closedir(d);

    info->num_interfaces = n;
    return WIFI_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////
void lowi_wifi_event_loop(lowi_wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    if (info->in_event_loop) {
        return;
    } else {
        info->in_event_loop = true;
    }

    pollfd pfd[2];
    memset(&pfd[0], 0, sizeof(pollfd) * 2);

    pfd[0].fd = nl_socket_get_fd(info->event_sock);
    pfd[0].events = POLLIN;
    pfd[1].fd = info->cleanup_socks[1];
    pfd[1].events = POLLIN;

    char buf[2048];
    /* TODO: Add support for timeouts */

    do {
        int timeout = -1;                   /* Infinite timeout */
        pfd[0].revents = 0;
        pfd[1].revents = 0;
        // ALOGI("Polling socket");
        int result = TEMP_FAILURE_RETRY(poll(pfd, 2, timeout));
        if (result < 0) {
            // ALOGE("Error polling socket");
        } else if (pfd[0].revents & POLLERR) {
            ALOGE("POLL Error; error no = %d (%s)", errno, strerror(errno));
            ssize_t result2 = TEMP_FAILURE_RETRY(read(pfd[0].fd, buf, sizeof(buf)));
            ALOGE("Read after POLL returned %zd, error no = %d (%s)", result2,
                  errno, strerror(errno));
        } else if (pfd[0].revents & POLLHUP) {
            ALOGE("Remote side hung up");
            break;
        } else if (pfd[0].revents & POLLIN) {
            // ALOGI("Found some events!!!");
            internal_pollin_handler(handle);
        } else if (pfd[1].revents & POLLIN) {
            memset(buf, 0, sizeof(buf));
            ssize_t result2 = TEMP_FAILURE_RETRY(read(pfd[1].fd, buf, sizeof(buf)));
            ALOGE("%s: Read after POLL returned %zd, error no = %d (%s)", __FUNCTION__,
                   result2, errno, strerror(errno));
            if (strncmp(buf, "Exit", 4) == 0) {
                ALOGD("Got a signal to exit!!!");
                if (TEMP_FAILURE_RETRY(write(pfd[1].fd, "Done", 4)) < 1) {
                    ALOGE("could not write to the cleanup socket");
                }
                break;
            } else {
                ALOGD("Rx'ed %s on the cleanup socket\n", buf);
            }
        } else {
            ALOGE("Unknown event - %0x, %0x", pfd[0].revents, pfd[1].revents);
        }
    } while (!info->clean_up);
    ALOGI("Exit %s", __FUNCTION__);
}
