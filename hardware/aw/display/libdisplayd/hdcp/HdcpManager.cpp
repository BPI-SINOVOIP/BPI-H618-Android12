

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "debug.h"
#include "HdcpManager.h"

#define FIRMWARE_PATH "/vendor/etc/hdcp/esm.fex"
#define SYSPATH_HDCP_ENABLE "/sys/devices/virtual/hdmi/hdmi/attr/hdcp_enable"
#define SYSPATH_HDCP_STATUS "/sys/devices/virtual/hdmi/hdmi/attr/hdcp_status"
#define SYSPATH_HDCP_TYPE "/sys/devices/virtual/hdmi/hdmi/attr/hdcp_type"

HdcpManager::HdcpManager()
    : mFirmwareReady(false), mHdcpEnabled(false)
{
    loadFirmware(FIRMWARE_PATH);
}

static int read_from_file(const char *path, char *buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        dd_error("Could not open '%s', %s(%d)", path, strerror(errno), errno);
        return -errno;
    }
    ssize_t count = read(fd, buf, size);
    close(fd);
    return count;
}

static int write_to_file(const char *path, const char *buffer, int i) {
    int fd = open(path, O_WRONLY, 0);
    if (fd == -1) {
        dd_error("Could not open '%s', %s(%d)", path, strerror(errno), errno);
        return -1;
    }
    write(fd, buffer, i);
    close(fd);
    return 0;
}

int HdcpManager::configHdcp(bool enable)
{
    if (enable && !mFirmwareReady) {
        loadFirmware(FIRMWARE_PATH);
    }

    int ret = write_to_file(SYSPATH_HDCP_ENABLE, enable ? "1":"0", 1);
    if (ret == 0) {
        mHdcpEnabled = enable;
    }
    return ret;
}

HdcpManager::HdcpLevel HdcpManager::getConnectedHdcpLevel() const
{
    char type = 0;
    if (read_from_file(SYSPATH_HDCP_TYPE, &type, 1) == 1) {
        dd_info("hdcp type %d", type);
        if (type == 0)
            return HDCP_V1;
        else if (type == 1)
            return HDCP_V2_2;
        else
            return HDCP_UNKNOWN;
    }
    return HDCP_UNKNOWN;
}

HdcpManager::HdcpAuthorizedStatus HdcpManager::getAuthorizedStatus() const
{
    char state = 0;
    if (read_from_file(SYSPATH_HDCP_STATUS, &state, 1) == 1) {
        dd_info("hdcp status %d", state);
        if (state == 3) {
            return AUTHORIZED;
        }
    }
    return mHdcpEnabled ? ERROR : UN_AUTHORIZED;
}

#define HDCP22_LOAD_FIRMWARE 1
#define ESM_IMG_SIZE (200*1024)
int HdcpManager::loadFirmware(const char *fw)
{
    mFirmwareReady = false;
    char *esmimg = (char *)malloc(ESM_IMG_SIZE);
    if (esmimg == nullptr) {
        return -ENOMEM;
    }
    int length = read_from_file(fw, esmimg, ESM_IMG_SIZE);
    if (length > 0) {
        dd_info("read firmware '%s', size=%d", fw, length);
        int fd = open("/dev/hdmi", O_RDWR);
        if (fd < 0) {
            dd_error("Could not open hdmi device, %s(%d)", strerror(errno), errno);
            mFirmwareReady = false;
            return -errno;
        }
        unsigned long arg[3] = { 0 };
        arg[0] = (unsigned long)esmimg;
        arg[1] = length;
        if (ioctl(fd, HDCP22_LOAD_FIRMWARE, arg) == 0) {
            mFirmwareReady = true;
            dd_info("load hdcp2.2 firmware success (size=%d)", length);
        }
        close(fd);
    }
    return mFirmwareReady ? 0 : -1;
}

