/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <fcntl.h>
#include <UtilsDisp2.h>
#include "Debug.h"

using namespace sunxi;

// dev_composer defines -->
enum {
    // create a fence timeline
    HWC_NEW_CLIENT = 1,
    HWC_DESTROY_CLIENT,
    HWC_ACQUIRE_FENCE,
    HWC_SUBMIT_FENCE,
};
// dev_composer defines <--

UtilsDisp2:: UtilsDisp2()
{
    mFd = open("/dev/disp", O_RDWR);
    if (mFd < 0) {
        DLOGE("open /dev/disp failed");
    }
}

UtilsDisp2:: ~UtilsDisp2()
{
    if (mFd >= 0) {
        close(mFd);
    }
}

int UtilsDisp2::stopWriteBack(int hwid)
{
    unsigned long args[4];
    args[0] = hwid;
    args[1] = 0;
    args[2] = 0;
    if (ioctl(mFd, DISP_CAPTURE_STOP, args) < 0) {
        DLOGE("stop de capture failed!");
        return -1;
    }
    return 0;
}

int UtilsDisp2::startWriteBack(int hwid)
{
    unsigned long args[4];
    args[0] = hwid;
    args[1] = 0;
    args[2] = 0;
    if (ioctl(mFd, DISP_CAPTURE_START, args) < 0) {
        DLOGE("start de capture failed!");
        return -1;
    }
    return 0;
}

int UtilsDisp2::switchToDefault(int hwid, int type, int mode)
{
    unsigned long args[4];
    args[0] = hwid;
    args[1] = type;
    args[2] = mode;
    if (ioctl(mFd, DISP_DEVICE_SWITCH, args) < 0) {
        DLOGE("failed! type=%d, mode=%d", type, mode);
        return -1;
    }
    return 0;
}

int UtilsDisp2::setDeviceConfigToDefault(int hwid, int type, int mode)
{
    unsigned long args[4];
    struct disp_device_config conf;

    memset(&conf, 0, sizeof(struct disp_device_config));
    conf.type = (enum disp_output_type)type;
    conf.mode = (enum disp_tv_mode)mode;
    conf.format = DISP_CSC_TYPE_YUV444;
    conf.cs = DISP_BT709;
    conf.bits = DISP_DATA_8BITS;
    conf.eotf = DISP_EOTF_GAMMA22;
    conf.dvi_hdmi = DISP_HDMI;
    conf.range = DISP_COLOR_RANGE_16_255;
    conf.scan = DISP_SCANINFO_NO_DATA;

    args[0] = hwid;
    args[1] = (unsigned long)&conf;
    if (ioctl(mFd, DISP_DEVICE_SET_CONFIG, (void*)args) < 0) {
        DLOGE("failed! type=%d, mode=%d", type, mode);
        return -1;
    }

    return 0;
}

int UtilsDisp2::getDeviceConfig(int hwid, struct disp_device_config* config)
{
    unsigned long arg[4];
    if (config == NULL) {
        return -1;
    }
    arg[0] = hwid;
    arg[1] = (unsigned long)config;
    if (ioctl(mFd, DISP_DEVICE_GET_CONFIG, (void*)arg) < 0) {
        DLOGE("failed!");
        return -1;
    }
    return 0;
}

int UtilsDisp2::captureCommit2(int hwid, struct disp_capture_info2* info)
{
    unsigned long arg[4];
    arg[0] = hwid;
    arg[1] = (unsigned long)info;
    arg[2] = 0;
    arg[3] = 0;
    if (ioctl(mFd, DISP_CAPTURE_COMMIT2, (void*)arg) < 0) {
        DLOGE("failed!");
        return -1;
    }
    return 0;
}

int UtilsDisp2::rtwbCommit(int hwid, unsigned int syncnum, disp_layer_config2* conf,
                           int lyrNum, struct disp_capture_info2* info)
{
    unsigned long args[4] = {0};

    args[0] = hwid;
    args[1] = 1;
    if (ioctl(mFd, DISP_SHADOW_PROTECT, (unsigned long)args)) {
        DLOGE("ioctl DISP_SHADOW_PROTECT error");
        return -1;
    }

    args[0] = hwid;
    args[1] = (unsigned long)conf;
    args[2] = lyrNum;
    args[3] = (unsigned long)info;
    if (ioctl(mFd, DISP_RTWB_COMMIT, (void *)args) < 0 ) {
        DLOGE("failed ");
        return -1;
    }

    args[0] = hwid;
    args[1] = HWC_SUBMIT_FENCE;
    args[2] = syncnum;
    args[3] = 0;
    if (ioctl(mFd, DISP_HWC_COMMIT, (unsigned long)args)) {
        DLOGE("ioctl DISP_HWC_COMMIT error");
    }

    args[0] = hwid;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    if (ioctl(mFd, DISP_SHADOW_PROTECT, (unsigned long)args)) {
        DLOGE("ioctl DISP_SHADOW_PROTECT error");
        return -1;
    }

    return 0;
}

