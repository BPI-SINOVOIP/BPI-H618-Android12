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

#include <cutils/properties.h>
#include "sunxi_typedef.h"
#include "device/device_controler.h"
#include "platform.h"
#include "utils.h"
#include "debug.h"

const char *type2name(int type)
{
    switch (type) {
    case DISP_OUTPUT_TYPE_HDMI: return "hdmi";
    case DISP_OUTPUT_TYPE_TV:   return "cvbs";
    case DISP_OUTPUT_TYPE_LCD:  return "lcd";
    default:
        dd_error("unknow type");
        return "unknow";
    }
}

static void updateDisplayConfigToBoot(const disp_device_config& config)
{
    char value[64];

    if (config.type == DISP_OUTPUT_TYPE_HDMI) {
        sprintf(value, "%d,%d - %d,%d,%d,%d",
                config.type, config.mode, config.format,
                config.bits, config.cs, config.eotf);
        property_set("vendor.sys.disp_config", value);
        sprintf(value, "%d,%d@", config.type, config.mode);
        property_set("vendor.sys.disp_rsl_fex", value);
        dd_debug("Update config to boot");
    }

    // save device config to /Resolve0::disp_rsl.fex
    saveDeviceConfigToFile(config);
}

DeviceControler::DeviceControler(
        int displayEngineIndex, int type, sunxi::IHWCPrivateService& client)
  : HardwareCtrl(displayEngineIndex),
    mClient(client),
    mType(type),
    mDeviceName(type2name(type)),
    mLogicalId(-1),
    mConnectState(0),
    mDeviceConfig(_device_congi_property(type2name(type))),
    mMargin(_margin_property(type2name(type))),
    mDataspace(_dataspace_property(type2name(type))),
    mPixelformat(_pixelformat_property((type2name(type)))),
    mAspectRatio(_aspect_ratio_property(type2name(type))),
    m3dLayerMode(LAYER_2D_ORIGINAL)
{
    mDeviceConfig.setHook(updateDisplayConfigToBoot);
}

DeviceControler::~DeviceControler()
{
}

void DeviceControler::present()
{
    /* Device offline */
    if (mConnectState.getValue() != 1)
        return;

    int m[4] = {0};
    Margin margin;
    getDisplayMargin(m);

    margin.left   = m[0];
    margin.right  = m[1];
    margin.top    = m[2];
    margin.bottom = m[3];

    if (mClient.setMargin(mLogicalId,
            margin.left, margin.right, margin.top, margin.bottom) != 0) {
        dd_error("[%s] setDisplayMargin: set margin error", mDeviceName);
    }

    /* Unblank and preset */
    mClient.blank(mLogicalId, 0);
    dd_info("Turen on display(%d), margin(%3d,%3d,%3d,%3d)",
            mLogicalId, margin.left, margin.right, margin.top, margin.bottom);
}

void DeviceControler::updateDisplayAttribute(struct disp_device_config *config)
{
    mDeviceConfig.save(*config);
}

int DeviceControler::setLogicalId(int id)
{
    mLogicalId = id;
    return 0;
}

int DeviceControler::setConnectState(int connected)
{
    mConnectState.setPending(connected);
    return 0;
}

int DeviceControler::setPixelFormat(int format)
{
    if (format != PIXEL_FORMAT_YUV444_8bit) {
        dd_error("[%s] setPixelFormat: unsupport pixel format %d",
                 mDeviceName, format);
        return -1;
    }
    mPixelformat.save((pixelformat_t)format);
    return 0;
}

int DeviceControler::setDataspaceMode(int mode)
{
    if (mode != DATASPACE_MODE_SDR) {
        dd_error("[%s] setDataspace: unsupport dataspace %d",
                 mDeviceName, mode);
        return -1;
    }
    mDataspace.save((dataspace_t)mode);
    return 0;
}

int DeviceControler::onDataspaceChange(int /* dataspace */) {
    return 0;
}

int DeviceControler::setDisplayModeImpl(int mode)
{
    return setMode(mode);
}

int DeviceControler::setDisplayMode(int mode)
{
    if (mDeviceConfig.get().mode == mode && LAYER_2D_ORIGINAL == m3dLayerMode)
        return 0;

    if (!isSupportMode(mode)) {
        dd_error("[%s] setDisplayMode: not support mode %d",
                 mDeviceName, mode);
        return -1;
    }

    int ret = 0;
    mClient.blank(mLogicalId, 1);
    ret = setDisplayModeImpl(mode);

    if (!ret) {
        /* Notify HWC to update screen window */
        mClient.setOutputMode(mLogicalId, mType, mode);
    }
    mClient.blank(mLogicalId, 0);

    if (ret != 0) {
        dd_error("[%s] setDisplayMode: HWC service return error", mDeviceName);
        return -1;
    }
    struct disp_device_config config;
    getDeviceConfig(&config);
    /*actuallly we donnot want 3d mode be save*/
    if (DISP_TV_MOD_1080P_24HZ_3D_FP != mode) {
	    mDeviceConfig.save(config);
    }
    return 0;
}

int DeviceControler::setDisplayMargin(int l, int r, int t, int b)
{
    Margin margin;
    margin.left   = MAX(MIN_MARGIN, MIN(l, MAX_MARGIN));
    margin.right  = MAX(MIN_MARGIN, MIN(r, MAX_MARGIN));
    margin.top    = MAX(MIN_MARGIN, MIN(t, MAX_MARGIN));
    margin.bottom = MAX(MIN_MARGIN, MIN(b, MAX_MARGIN));

    if (mMargin.get() == margin)
        return 0;

    if (mClient.setMargin(mLogicalId,
            margin.left, margin.right, margin.top, margin.bottom) != 0) {
        dd_error("[%s] setDisplayMargin: set margin error", mDeviceName);
        return -1;
    }

    mMargin.save(margin);
    return 0;
}

int DeviceControler::setAspectRatio(int ratio)
{
    if (mAspectRatio.get() == ratio)
        return 0;

    if (mClient.setVideoRatio(mLogicalId, ratio) != 0) {
        dd_error("[%s] setVideoRatio: set video ratio error", mDeviceName);
        return -1;
    }

    mAspectRatio.save((aspect_ratio_t)ratio);
    return 0;
}

int DeviceControler::getAspectRatio()
{
    return static_cast<int>(mAspectRatio.get());
}

int DeviceControler::getHdmiConnectState(int type)
{
    return platformGetDeviceConnectState(type);
}

int DeviceControler::getDisplayMode()
{
#if 0
    int mode = getMode();
    if (mode != mDeviceConfig.get().mode) {
        dd_error("[%s] Saved attribute is not match,"
                 " saved mode=%d, actual mode=%d",
                 mDeviceName, mDeviceConfig.get().mode, mode);
        // TODO:
        // A bug in disp2 driver:
        //   If you get the mode immediately after setting the device configuration,
        //   the returned mode is 0.
        if (mType == DISP_OUTPUT_TYPE_HDMI) {
            dd_info("!!! use saved mode(%d) for hdmi.", mDeviceConfig.get().mode);
            return mDeviceConfig.get().mode;
        }
    }
    return mode;
#else
    return getMode();
#endif
}

int DeviceControler::getPixelFormat()
{
    return static_cast<int>(mPixelformat.get());
}

int DeviceControler::getDataspaceMode()
{
    return static_cast<int>(mDataspace.get());
}

int DeviceControler::getCurrentDataspace()
{
    /* TODO */
    return 0;
}

int DeviceControler::getDisplayMargin(int *margin)
{
    *margin++ = mMargin.get().left;
    *margin++ = mMargin.get().right;
    *margin++ = mMargin.get().top;
    *margin++ = mMargin.get().bottom;
    return 0;
}

static bool is3dLayerMode(int mode) {
    switch (mode) {
    case LAYER_3D_LEFT_RIGHT_HDMI:
    case LAYER_3D_TOP_BOTTOM_HDMI:
    case LAYER_3D_DUAL_STREAM:
    case LAYER_3D_LEFT_RIGHT_ALL:
    case LAYER_3D_TOP_BOTTOM_ALL:
        return true;
    default:
        return false;
    }
}

bool DeviceControler::isCurrent3DOutput()
{
    return is3dLayerMode(m3dLayerMode);
}

int DeviceControler::get3DLayerMode()
{
    return m3dLayerMode;
}

int DeviceControler::set3DLayerMode(int mode)
{
    dd_info("Request layer mode: %s", toString((output_layer_mode_t)mode));

    if (mType == DISP_OUTPUT_TYPE_HDMI) {
        if (is3dLayerMode(mode)) {
            if (isSupport3D() == false) {
                dd_error("HDMI sink not support 3d display");
                return -1;
            }
            // FIXME: add more 3d output mode support...
            if (setDisplayMode(DISP_TV_MOD_1080P_24HZ_3D_FP) != 0) {
                dd_error("set 3d output failed");
                return -1;
            }
	    mClient.set3DMode(mLogicalId, mode);
	} else {
		mClient.set3DMode(mLogicalId, mode);
		setDisplayMode(mDeviceConfig.get().mode);
	}
	m3dLayerMode = mode;
    } else {
	    switch (mode) {
		    case LAYER_3D_LEFT_RIGHT_HDMI:
			    m3dLayerMode = LAYER_2D_LEFT;
			    break;
		    case LAYER_3D_TOP_BOTTOM_HDMI:
			    m3dLayerMode = LAYER_2D_TOP;
			    break;
		    case LAYER_3D_DUAL_STREAM:
			    m3dLayerMode = LAYER_2D_DUAL_STREAM;
			    break;
	    }
	    mClient.set3DMode(mLogicalId, m3dLayerMode);
    }

    return 0;
}

void DeviceControler::onDeviceReconnected()
{
    if (mType == DISP_OUTPUT_TYPE_HDMI) {
        dd_info("reset layer mode: %s", toString((output_layer_mode_t)m3dLayerMode));
        set3DLayerMode(m3dLayerMode);
    }
}

void DeviceControler::hwcBlank(int blank)
{
    mClient.blank(mLogicalId, blank);
}

void DeviceControler::hwcSetDataspace(int dataspace)
{
    mClient.setDataspace(mLogicalId, dataspace);
    dd_error("hwcSetDataspace: %08x", dataspace);
}

void DeviceControler::dump(android::String8& str)
{
    str.appendFormat("    Name: %s (logicalId = %d, type = %d)\n", mDeviceName, mLogicalId, mType);
    str.appendFormat("    Connected(%d)\n", mConnectState.getValue());

    android::String8 config, margin, dataspace, pixelformat, aspectratio;
    mDeviceConfig.dump(config);
    mMargin.dump(margin);
    mDataspace.dump(dataspace);
    mPixelformat.dump(pixelformat);
    mAspectRatio.dump(aspectratio);
    str.appendFormat("    Attrbute:\n");
    str.appendFormat("      - %s\n"
                     "      - %s\n"
                     "      - %s\n"
                     "      - %s\n"
                     "      - %s\n", config.string(), margin.string(),
                     dataspace.string(), pixelformat.string(), aspectratio.string());
    android::String8 local;
    dumpHook(local);
    str.appendFormat("%s\n", local.string());
}

