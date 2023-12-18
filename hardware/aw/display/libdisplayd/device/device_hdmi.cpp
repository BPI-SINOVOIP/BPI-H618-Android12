/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <hardware/hwcomposer.h>

#include "debug.h"
#include "utils.h"
#include "platform.h"
#include "device/device_hdmi.h"
//#define USE_EDID_NATIVE_MODE
#define DEFALUT_MODE_USE_UBOOT

HdmiDevice::HdmiDevice(int displayEngineIndex, sunxi::IHWCPrivateService& client)
  : DeviceControler(displayEngineIndex, DISP_OUTPUT_TYPE_HDMI, client),
    mRequestDataspace(EdidStrategy::SDR)
{
    dd_info("Device hdmi init");
}

HdmiDevice::~HdmiDevice()
{

}

int HdmiDevice::getSupportModeList(std::vector<int>& out)
{
    for (size_t i = 0; i < NELEM(_hdmi_supported_modes); i++) {
        int mode = _hdmi_supported_modes[i];
        if (isSupportMode(mode))
            out.push_back(mode);
    }
    return 0;
}

int HdmiDevice::getSupportVendorPreferenceMode(int *mode)
{
    int perfect = DEFAULT_HDMI_OUTPUT_MODE;
    for (size_t i = 0; i < NELEM(_hdmi_perfect_modes); i++) {
        if (isSupportMode(_hdmi_perfect_modes[i])) {
            perfect = _hdmi_perfect_modes[i];
            break;
        }
    }
    *mode = perfect;
    return 0;
}

int HdmiDevice::getSupportPixelFormat(std::vector<int>& out)
{
    out.clear();
    return 0;
}

int HdmiDevice::getSupportDataspace(std::vector<int>& out)
{
    out.clear();
    return 0;
}

bool HdmiDevice::isSupportMode(int mode)
{
    if (checkModeSupport(mode) == 1)
        return true;
    return false;
}

int HdmiDevice::setDisplayModeImpl(int mode)
{
    struct disp_device_config config;
    getDeviceConfig(&config);

    int inSupportedList = false;
    for (size_t i = 0; i < NELEM(_hdmi_supported_modes); i++) {
        if (_hdmi_supported_modes[i] == mode) {
            inSupportedList = true;
            break;
        }
    }
    for (size_t i = 0; i < NELEM(_hdmi_supported_3d_modes); i++) {
        if (_hdmi_supported_3d_modes[i] == mode) {
            inSupportedList = true;
            break;
        }
    }

    if (!inSupportedList) {
        dd_error("setDisplayModeImpl: mode %d not in supported list", mode);
        return -1;
    }

    /* Verify Device config according sink edid */
    config.mode = (disp_tv_mode)mode;
    mEdidStrategy.update();
    mEdidStrategy.checkAndCorrectDeviceConfig(&config);
    setDeviceConfig(&config);
    dumpDeviceConfig("New config", &config);
    return 0;
}

int HdmiDevice::setDataspaceMode(int mode)
{
    int dataspace = DATASPACE_SDR;
    bool reconfig = false;
    struct disp_device_config config;
    getDeviceConfig(&config);

    if (mode == DATASPACE_MODE_AUTO
        && mRequestDataspace != EdidStrategy::SDR) {
        mRequestDataspace = EdidStrategy::SDR;
    }
    if ((mode == DATASPACE_MODE_SDR) ||
            (mode == DATASPACE_MODE_AUTO &&
             mRequestDataspace == EdidStrategy::SDR)) {
        if (config.eotf != DISP_EOTF_GAMMA22) {
            dd_debug("setDataspaceMode: HDR --> SDR");
            dataspace = DATASPACE_SDR;
            config.eotf = DISP_EOTF_GAMMA22;
            reconfig = true;
        }
    } else if ((mode == DATASPACE_MODE_HDR) ||
            (mode == DATASPACE_MODE_AUTO &&
             mRequestDataspace == EdidStrategy::HDR)) {
        if (mEdidStrategy.supportedHDR() == false) {
            dd_error("setDataspaceMode: sink not support hdr");
            return -1;
        }
        if (config.eotf != DISP_EOTF_SMPTE2084) {
            dd_debug("setDataspaceMode: SDR --> HDR");
            dataspace = DATASPACE_HDR;
            config.eotf = DISP_EOTF_SMPTE2084;
            reconfig = true;
        }
    } else if ((mode == DATASPACE_MODE_HDRP) ||
            (mode == DATASPACE_MODE_AUTO &&
             mRequestDataspace == EdidStrategy::HDRP)) {
        if (mEdidStrategy.supportedHDR10P() == false) {
            dd_error("setDataspaceMode: sink not support hdr10+");
            return -1;
        }
        if (config.eotf != DISP_EOTF_SMPTE2084) {
            dd_debug("setDataspaceMode: SDR --> HDR10+");
            dataspace = DATASPACE_HDRP;
            config.eotf = DISP_EOTF_SMPTE2084;
            reconfig = true;
        }
    } else if ((mode == DATASPACE_MODE_DV) ||
            (mode == DATASPACE_MODE_AUTO &&
             mRequestDataspace == EdidStrategy::DV)) {
        if (mEdidStrategy.supportedHDR() == false) {
            dd_error("setDataspaceMode: sink not support dv");
            return -1;
        }
        if (config.eotf != DISP_EOTF_SMPTE2084) {
            dd_debug("setDataspaceMode: SDR --> dv");
            dataspace = DATASPACE_DV;
            config.eotf = DISP_EOTF_SMPTE2084;
            reconfig = true;
        }
    }

    hwcSetDataspace(dataspace);

    if (reconfig) {
        hwcBlank(1);
        mEdidStrategy.checkAndCorrectDeviceConfig(&config);
        setDeviceConfig(&config);
        hwcBlank(0);
    }

    mDataspace.save((dataspace_t)mode);
    return 0;
}

int HdmiDevice::onDataspaceChange(int dataspace)
{
    if (mDataspace.get() != DATASPACE_MODE_AUTO) {
        dd_error("Current dataspace mode not equal to auto mode");
        return 0;
    }

    struct disp_device_config config;
    getDeviceConfig(&config);

    int dirty = 0;
    int request = EdidStrategy::SDR;

    if (DATASPACE_HDR == dataspace) {
        request = EdidStrategy::HDR;
        dd_info("Rqeuest HDR output");
    } else if (DATASPACE_WCG == dataspace) {
        request = EdidStrategy::WCG;
        dd_info("Rqeuest WCG output");
    } else if (DATASPACE_HDRP == dataspace) {
        request = EdidStrategy::HDRP;
        //dd_info("Rqeuest hdr10+ output");
    } else if (DATASPACE_DV == dataspace) {
        request = EdidStrategy::DV;
        dd_info("Rqeuest dv output");
    } else {
        dataspace = DATASPACE_SDR;
        request = EdidStrategy::SDR;
        dd_info("Rqeuest SDR output");
    }
    mRequestDataspace = request;

    if (mEdidStrategy.setupConfig(request, &config, &dirty)) {
        dd_info("setupConfig success, dirty = %d", dirty);

        // Notify hwc to respone for HDR or SDR
        hwcSetDataspace(dataspace);

        if (dirty > 0) {
            hwcBlank(1);
            setDeviceConfig(&config);
            hwcBlank(0);
        }
        return 0;
    }

    // Not support request dataspace mode
    return -1;
}

int HdmiDevice::performOptimalMode()
{
    int savedmode = mDeviceConfig.get().mode;
    int perfect;

    if (getHdmiUserSetting() == HDMI_AUTO_MODE) {
        dd_info("performOptimalMode: hdmi mode user setting auto, try perfect modes");
        goto try_perfect_mode;
    }

    if (mDeviceConfig.get().type != DISP_OUTPUT_TYPE_HDMI) {
        dd_info("performOptimalMode: saved mode is invalid, try perfect modes");
        goto try_perfect_mode;
    }
    if (isSupportMode(savedmode)) {
        dd_info("performOptimalMode: use saved hdmi mode(%d)", savedmode);
        return savedmode;
    }

try_perfect_mode:
    getSupportVendorPreferenceMode(&perfect);
    dd_info("performOptimalMode: use perfect hdmi mode(%d)", perfect);
    return perfect;
}

void HdmiDevice::userSetting2Config(disp_device_config *config)
{
    config->type = DISP_OUTPUT_TYPE_HDMI;
    config->mode = (enum disp_tv_mode)performOptimalMode();

    switch (mPixelformat.get()) {
    case PIXEL_FORMAT_RGB888_8bit:
        config->format = DISP_CSC_TYPE_RGB;
        config->bits   = DISP_DATA_8BITS;
        break;
    case PIXEL_FORMAT_YUV422_10bit:
        config->format = DISP_CSC_TYPE_YUV422;
        config->bits   = DISP_DATA_10BITS;
        break;
    case PIXEL_FORMAT_YUV420_10bit:
        config->format = DISP_CSC_TYPE_YUV420;
        config->bits   = DISP_DATA_10BITS;
        break;
    case PIXEL_FORMAT_YUV444_8bit:
    default:
        config->format = DISP_CSC_TYPE_YUV444;
        config->bits   = DISP_DATA_8BITS;
        break;
    }

    switch (mDataspace.get()) {
    case DATASPACE_MODE_SDR:
    case DATASPACE_MODE_AUTO:
        config->eotf = DISP_EOTF_GAMMA22;
        break;
    case DATASPACE_MODE_HDR:
        config->eotf = DISP_EOTF_SMPTE2084;
        break;
    default:
        config->eotf = DISP_EOTF_GAMMA22;
        break;
    }
}

int HdmiDevice::performOptimalConfig(int enable)
{
    if (enable == 0) {
        mConnectState.setPending(0);
        mConnectState.latch();
        return 0;
    }

    int connected = platformGetDeviceConnectState(mType);
    mConnectState.setPending(connected);

    if (!mConnectState.isDirty() || !connected) {
        if (enable) {
            // when hdmi enable as primary display but not actual open yet,
            // we need to open it manually, or the display driver will not consume any input frames
            disp_device_config config;
            getDeviceConfig(&config);
            if (config.type != DISP_OUTPUT_TYPE_HDMI) {
                dd_error("force open hdmi as primary display");
                userSetting2Config(&config);
                mEdidStrategy.update();
                mEdidStrategy.checkAndCorrectDeviceConfig(&config);
                setDeviceConfig(&config);
            }
        }
        dd_error("hdmi connected state is %d", connected);
        mConnectState.latch();
        return 0;
    }

    int retry = 3;

__verify_device_config:
    /* Read and parse edid */
    mEdidStrategy.update();

    disp_device_config config;
    userSetting2Config(&config);
    mEdidStrategy.checkAndCorrectDeviceConfig(&config);
    int ret = setDeviceConfig(&config);
    if ( ret != 0) {
        dd_error("setDeviceConfig error(%d), driver not ready yet!", ret);
        if (retry-- > 0) {
            usleep(1000 * 100);
            goto __verify_device_config;
        }
    }

    dumpDeviceConfig("Device config", &config);
    updateDisplayAttribute(&config);

    mConnectState.setPending(1);
    mConnectState.latch();
    return 0;
}

int HdmiDevice::performDefaultConfig(int enable)
{
    if (enable == 0) {
        mConnectState.setPending(0);
        mConnectState.latch();
        return 0;
    }
    if (platformGetDeviceConnectState(mType) == 0) {
        /* Not connected yet, just return */
        dd_info("HDMI performDefaultConfig: not connected yet");
        return 0;
    }

    /* Read and parse edid */
    mEdidStrategy.update();

    int dirty = 0;
    struct disp_device_config config;
    struct disp_device_config newconfig;
    getDeviceConfig(&config);
    newconfig = config;

    if (config.type != DISP_OUTPUT_TYPE_HDMI) {
        dirty++;
        newconfig.type = DISP_OUTPUT_TYPE_HDMI;
        dd_info("Current output type (%d), switch to hdmi output", config.type);
    }

#ifdef DEFALUT_MODE_USE_UBOOT
// if uboot mode support, use uboot mode
    if (checkModeSupport(config.mode) == 0) {
        dirty++;
        newconfig.mode = (enum disp_tv_mode)performOptimalMode();
        dd_info("Current mode (%d) not supported, change to new mode (%d)",
                config.mode, newconfig.mode);
    }
#else
// first try saved mode, if saved mode is not supported or not found, use mode in _hdmi_perfect_modes
    newconfig.mode = (enum disp_tv_mode)performOptimalMode();
    if (config.mode != newconfig.mode) {
        dirty++;
        dd_info("Current mode (%d) not equal to defalut mode, change to defalut mode (%d)",
              config.mode, newconfig.mode);
    }
#endif
    /* Verify pixelformat/colorspace/bits/eotf with sink edid */
    dirty += mEdidStrategy.checkAndCorrectDefaultDeviceConfig(&newconfig);

    /* select a default eotf herer */
    if (newconfig.eotf != DISP_EOTF_GAMMA22 &&
            newconfig.eotf != DISP_EOTF_SMPTE2084) {
        newconfig.eotf = DISP_EOTF_GAMMA22;
        dirty++;
    }

    if (dirty) {
        hwcBlank(1);
        setDeviceConfig(&newconfig);
        hwcBlank(0);
    }
    dd_info("performDefaultConfig dirty=%d", dirty);
    dumpDeviceConfig("Old config", &config);
    dumpDeviceConfig("New config", &newconfig);
    updateDisplayAttribute(&newconfig);

    mConnectState.setPending(1);
    mConnectState.latch();
    return 0;
}

int HdmiDevice::getHDMIVersionFromEdid(void)
{
    return mEdidStrategy.getHDMIVersion();
}

int HdmiDevice::setPixelFormat(int format)
{
    switch (format) {
    case PIXEL_FORMAT_YUV422_10bit:
        mPerfectFormat = DISP_CSC_TYPE_YUV422;
        mPerfectDepth  = DISP_DATA_10BITS;
        break;
    case PIXEL_FORMAT_YUV420_10bit:
        mPerfectFormat = DISP_CSC_TYPE_YUV420;
        mPerfectDepth  = DISP_DATA_10BITS;
        break;
    case PIXEL_FORMAT_YUV444_8bit:
        mPerfectFormat = DISP_CSC_TYPE_YUV444;
        mPerfectDepth  = DISP_DATA_8BITS;
        break;
    case PIXEL_FORMAT_RGB888_8bit:
        mPerfectFormat = DISP_CSC_TYPE_RGB;
        mPerfectDepth  = DISP_DATA_8BITS;
        break;
    case PIXEL_FORMAT_AUTO:
    default:
        mPerfectFormat = -1;
        mPerfectDepth  = -1;
        break;
    }

    struct disp_device_config config;
    getDeviceConfig(&config);

    /* perform the best config for auto mode */
    if (mPerfectFormat == -1 || mPerfectDepth == -1) {
        /* TODO */
        mPixelformat.save((pixelformat_t)PIXEL_FORMAT_AUTO);
        return 0;
    }

    if (mEdidStrategy.supportedPixelFormat(config.mode, mPerfectFormat, mPerfectDepth)) {
        if (config.format != mPerfectFormat || config.bits != mPerfectDepth) {
            config.format = (disp_csc_type)mPerfectFormat;
            config.bits = (disp_data_bits)mPerfectDepth;
            config.range = (config.format == DISP_CSC_TYPE_RGB) ? DISP_COLOR_RANGE_0_255 : DISP_COLOR_RANGE_16_235;
            setDeviceConfig(&config);
            updateDisplayAttribute(&config);
            dumpDeviceConfig("New config", &config);
        }
        mPixelformat.save((pixelformat_t)format);
        return 0;
    }
    return -1;
}

bool HdmiDevice::isSupport3D()
{
    // FIXME: add more 3d output mode...
    return (isSupportMode(DISP_TV_MOD_1080P_24HZ_3D_FP) && mEdidStrategy.supported3DPresent());
}

void HdmiDevice::dumpDeviceConfig(const char *prefix, struct disp_device_config *config)
{
    dd_info("%s: type[%d] mode[%d] pixelformat[%d] bits[%d] eotf[%d] colorspace[%d]",
            prefix,
            config->type, config->mode, config->format, config->bits,
            config->eotf, config->cs);
}

void HdmiDevice::dumpHook(android::String8& str)
{
    mEdidStrategy.dump(str);
}

/**
 * @name       :getHDMINativeMode
 * @brief      :get best resolution from edid
 * @param[IN]  :NONE
 * @param[OUT] :NONE
 * @return     :resolution mode id(dispmode)
 */
int HdmiDevice::getHDMINativeMode()
{
    int vendor;
#ifdef USE_EDID_NATIVE_MODE
    mEdidStrategy.update();
    vendor = mEdidStrategy.getNative();
    if (vendor < 0) {
        // find a default mode as native mode
        dd_info("Can't not find natvie mode:%d\n", vendor);
        int defaultModes[4] = {
            DISP_TV_MOD_1080P_60HZ, DISP_TV_MOD_1080P_50HZ,
            DISP_TV_MOD_720P_60HZ, DISP_TV_MOD_720P_50HZ,
        };

        for (int i = 0; i < 4; i++) {
            if (isSupportMode(defaultModes[i]) == true) {
                vendor = defaultModes[i];
                break;
            }
        }
        if (vendor < 0) {
            dd_error("can't fine any supported native mode");
            vendor = DISP_TV_MOD_720P_60HZ;
        }
    }
#else
    //use vendor preference mode as native mode return
    getSupportVendorPreferenceMode(&vendor);
    dd_info("getHDMINativeMode: use perfect hdmi mode(%d) as native mode", vendor);
#endif
    return vendor;
}
