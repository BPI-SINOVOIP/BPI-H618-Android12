


#include "debug.h"
#include "utils.h"
#include "platform.h"
#include "device/device_cvbs.h"

CvbsDevice::CvbsDevice(int displayEngineIndex, sunxi::IHWCPrivateService& client)
  : DeviceControler(displayEngineIndex, DISP_OUTPUT_TYPE_TV, client)
{
    dd_info("Device cvbs init");
}

CvbsDevice::~CvbsDevice()
{

}

int CvbsDevice::getSupportModeList(std::vector<int>& out)
{
    for (size_t i = 0; i < NELEM(_cvbs_supported_modes); i++) {
        int mode = _cvbs_supported_modes[i];
        if (isSupportMode(mode))
            out.push_back(mode);
    }
    return 0;
}


int CvbsDevice::getSupportPixelFormat(std::vector<int>& out)
{
    out.clear();
    return 0;
}

int CvbsDevice::getSupportDataspace(std::vector<int>& out)
{
    out.clear();
    return 0;
}

bool CvbsDevice::isSupportMode(int mode)
{
    for (size_t i = 0; i < NELEM(_cvbs_supported_modes); i++) {
        if (mode == _cvbs_supported_modes[i])
            return true;
    }
    return false;
}

int CvbsDevice::performOptimalConfig(int enable)
{
    return performDefaultConfig(enable);
}

int CvbsDevice::performDefaultConfig(int enable)
{
    if (enable == 0) {
        mConnectState.setPending(0);
        mConnectState.latch();
        return 0;
    }

    if (platformGetDeviceConnectState(mType) == 0) {
        /* Not connected yet, just return */
        dd_info("CVBS performDefaultConfig: not connected yet");
        return 0;
    }

    int dirty = 0;
    int mode = mDeviceConfig.get().mode;
    struct disp_device_config config;
    struct disp_device_config newconfig;
    getDeviceConfig(&config);

    if (mode != DISP_TV_MOD_PAL && mode != DISP_TV_MOD_NTSC) {
        mode = DEFAULT_CVBS_OUTPUT_MODE;
        dd_info("The saved mode is invalid, Use default cvbs mode: %d", mode);
    }
    if (config.mode != mode || config.type != DISP_OUTPUT_TYPE_TV) {
        dirty++;
        newconfig.mode   = (enum disp_tv_mode)mode;
        newconfig.type   = DISP_OUTPUT_TYPE_TV;
        newconfig.format = DISP_CSC_TYPE_YUV444;
        newconfig.bits   = DISP_DATA_8BITS;
        newconfig.eotf   = DISP_EOTF_GAMMA22;
        newconfig.range  = DISP_COLOR_RANGE_16_235;
        dd_info("Current mode (%d) not supported, change to new mode (%d)",
                config.mode, newconfig.mode);
    }

    dd_info("performDefaultConfig dirty=%d", dirty);
    dumpDeviceConfig("Old config", &config);
    if (dirty) {
        setDeviceConfig(&newconfig);
        dumpDeviceConfig("New config", &newconfig);
        updateDisplayAttribute(&newconfig);
    }

    mConnectState.setPending(1);
    mConnectState.latch();
    return 0;
}

void CvbsDevice::dumpDeviceConfig(const char *prefix, struct disp_device_config *config)
{
    dd_info("%s: type[%d] mode[%d] pixelformat[%d] bits[%d] eotf[%d] colorspace[%d]",
            prefix,
            config->type, config->mode, config->format, config->bits,
            config->eotf, config->cs);
}

