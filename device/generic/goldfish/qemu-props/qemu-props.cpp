/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* this program is used to read a set of system properties and their values
 * from the emulator program and set them in the currently-running emulated
 * system. It does so by connecting to the 'boot-properties' qemud service.
 *
 * This program should be run as root and called from
 * /system/etc/init.ranchu.rc exclusively.
 */

#define LOG_TAG  "qemu-props"

#define DEBUG  1

#if DEBUG
#  include <log/log.h>
#  define  DD(...)    ALOGI(__VA_ARGS__)
#else
#  define  DD(...)    ((void)0)
#endif

#include <string_view>
#include <cutils/properties.h>
#include <unistd.h>
#include <qemu_pipe_bp.h>
#include <qemud.h>
#include <string.h>
#include <stdio.h>

/* Name of the qemud service we want to connect to.
 */
#define  QEMUD_SERVICE  "boot-properties"

#define  MAX_TRIES      5

#define QEMU_MISC_PIPE "QemuMiscPipe"

namespace {
// qemu-props will not set these properties.
const char* const k_properties_to_ignore[] = {
    "dalvik.vm.heapsize",
    "ro.opengles.version",
    "qemu.adb.secure",
    nullptr,
};

// These properties will not be prefixed with "vendor.".
const char* const k_system_properties[] = {
    "qemu.sf.lcd_density",
    "qemu.hw.mainkeys",
    nullptr,
};

bool check_if_property_in_list(const char* prop_name, const char* const* prop_list) {
    for (; *prop_list; ++prop_list) {
        if (!strcmp(prop_name, *prop_list)) {
            return true;
        }
    }
    return false;
}

// We don't want to rename properties which already have the prefix
// or the system properties.
bool need_prepend_prefix(const char* prop, const std::string_view prefix) {
    return strncmp(prefix.data(), prop, prefix.size()) &&
           !check_if_property_in_list(prop, k_system_properties);
}
}  // namespace

int s_QemuMiscPipe = -1;
void static notifyHostBootComplete();
void static sendHeartBeat();
void static sendMessage(const char* mesg);
void static closeMiscPipe();
extern void parse_virtio_serial();

int main(void)
{
    int  qemud_fd, count = 0;

    /* try to connect to the qemud service */
    {
        int  tries = MAX_TRIES;

        while (true) {
            qemud_fd = qemud_channel_open( "boot-properties" );
            if (qemud_fd >= 0)
                break;

            if (--tries <= 0) {
                DD("Could not connect after too many tries. Aborting");
                return 1;
            }

            DD("waiting 1s to wait for qemud.");
            sleep(1);
        }
    }

    DD("connected to '%s' qemud service.", QEMUD_SERVICE);

    /* send the 'list' command to the service */
    if (qemud_channel_send(qemud_fd, "list", -1) < 0) {
        DD("could not send command to '%s' service", QEMUD_SERVICE);
        return 1;
    }

    /* read each system property as a single line from the service,
     * until exhaustion.
     */
    while (true) {
#define  BUFF_SIZE   (PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX + 2)
        DD("receiving..");
        char* prop_value;
        char  temp[BUFF_SIZE];
        int   len = qemud_channel_recv(qemud_fd, temp, sizeof(temp) - 1);

        /* lone NUL-byte signals end of properties */
        if (len < 0 || len > (BUFF_SIZE - 1) || !temp[0]) {
            break;
        }

        temp[len] = '\0';  /* zero-terminate string */

        DD("received: %.*s", len, temp);

        /* separate propery name from value */
        prop_value = strchr(temp, '=');
        if (!prop_value) {
            DD("invalid format, ignored.");
            continue;
        }

        *prop_value = 0;
        ++prop_value;

        if (check_if_property_in_list(temp, k_properties_to_ignore)) {
            ALOGI("ignoring '%s' property", temp);
            continue;   // do not set these
        }

        char renamed_property[BUFF_SIZE];
        const char* final_prop_name = nullptr;

        using namespace std::literals;
        static constexpr std::string_view k_vendor_prefix = "vendor."sv;
        if (need_prepend_prefix(temp, k_vendor_prefix)) {
            snprintf(renamed_property, sizeof(renamed_property), "%.*s%s",
                     int(k_vendor_prefix.size()), k_vendor_prefix.data(), temp);

            final_prop_name = renamed_property;
        } else {
            final_prop_name = temp;
        }

        if (property_set(final_prop_name, prop_value) < 0) {
            ALOGW("could not set property '%s' to '%s'", final_prop_name, prop_value);
        } else {
            ALOGI("successfully set property '%s' to '%s'", final_prop_name, prop_value);
            count += 1;
        }
    }

    close(qemud_fd);

    parse_virtio_serial();

    char temp[BUFF_SIZE];
    sendHeartBeat();
    while (s_QemuMiscPipe >= 0) {
        usleep(5000000); /* 5 seconds */
        sendHeartBeat();
        property_get("vendor.qemu.dev.bootcomplete", temp, "");
        int is_boot_completed = (strncmp(temp, "1", 1) == 0) ? 1 : 0;
        if (is_boot_completed) {
            ALOGI("tell the host boot completed");
            notifyHostBootComplete();
            break;
        }
    }

    while (s_QemuMiscPipe >= 0) {
        usleep(30*1000000); /* 30 seconds */
        sendHeartBeat();
    }

    /* finally, close the channel and exit */
    closeMiscPipe();
    DD("exiting (%d properties set).", count);
    return 0;
}

void sendHeartBeat() {
    sendMessage("heartbeat");
}

void notifyHostBootComplete() {
    sendMessage("bootcomplete");
}

void sendMessage(const char* mesg) {
   if (s_QemuMiscPipe < 0) {
        s_QemuMiscPipe = qemu_pipe_open_ns(NULL, QEMU_MISC_PIPE, O_RDWR);
        if (s_QemuMiscPipe < 0) {
            ALOGE("failed to open %s", QEMU_MISC_PIPE);
            return;
        }
    }

    int32_t cmd_len = strlen(mesg) + 1; //including trailing '\0'
    qemu_pipe_write_fully(s_QemuMiscPipe, &cmd_len, sizeof(cmd_len));
    qemu_pipe_write_fully(s_QemuMiscPipe, mesg, cmd_len);

    int r = qemu_pipe_read_fully(s_QemuMiscPipe, &cmd_len, sizeof(cmd_len));
    if (r || (cmd_len < 0)) {
        closeMiscPipe();
        return;
    }

    while (cmd_len > 0) {
        char buf[64];
        const size_t chunk = std::min<size_t>(cmd_len, sizeof(buf));
        r = qemu_pipe_read_fully(s_QemuMiscPipe, buf, chunk);
        if (r) {
            closeMiscPipe();
            return;
        } else {
            cmd_len -= chunk;
        }
    }
}

void closeMiscPipe() {
    if (s_QemuMiscPipe >= 0) {
        close(s_QemuMiscPipe);
        s_QemuMiscPipe = -1;
    }
}
