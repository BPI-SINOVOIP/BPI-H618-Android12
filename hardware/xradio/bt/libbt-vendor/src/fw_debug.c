#include <stdio.h>
#define LOG_TAG "BT_FW_DBG"
#include <utils/Log.h>

int handle_fw_debug_info(uint8_t *pkg)
{
    char buf[512];
    uint8_t evt_code     = *pkg++;
    int32_t pkg_len      = *pkg++ - 1;
    uint8_t sub_evt_code = *pkg++;

    if (evt_code == 0xFF && sub_evt_code == 0x57 && pkg_len > 0) {
        memcpy(buf, pkg, pkg_len);
        buf[pkg_len] = 0;
        ALOGD("%s", buf);
        return 0;
    }
    ALOGE("Error PKG, evt: 0x%02X, sub_evt: 0x%02X, len: %d",
            evt_code, sub_evt_code, pkg_len);

    return -1;
}
