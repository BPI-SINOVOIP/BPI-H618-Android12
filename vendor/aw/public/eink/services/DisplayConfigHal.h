#pragma once

#include <ui/Rect.h>

#include <com/softwinner/IEinkMode.h>
#include <vendor/display/config/1.0/IDisplayConfig.h>

using ::com::softwinner::IEinkMode;
using ::vendor::display::config::V1_0::IDisplayConfig;
using ::vendor::display::config::V1_0::hwc2_eink_refresh_mode_t;

static std::map<int, hwc2_eink_refresh_mode_t> kModeMap = {
    {IEinkMode::INIT,  hwc2_eink_refresh_mode_t::HWC2_EINK_INIT_MODE},
    {IEinkMode::DU,    hwc2_eink_refresh_mode_t::HWC2_EINK_DU_MODE},
    {IEinkMode::GC16,  hwc2_eink_refresh_mode_t::HWC2_EINK_GC16_MODE},
    {IEinkMode::GC4,   hwc2_eink_refresh_mode_t::HWC2_EINK_GC4_MODE},
    {IEinkMode::A2,    hwc2_eink_refresh_mode_t::HWC2_EINK_A2_MODE},
    {IEinkMode::GL16,  hwc2_eink_refresh_mode_t::HWC2_EINK_GL16_MODE},
    {IEinkMode::GLR16, hwc2_eink_refresh_mode_t::HWC2_EINK_GLR16_MODE},
    {IEinkMode::GLD16, hwc2_eink_refresh_mode_t::HWC2_EINK_GLD16_MODE},
    {IEinkMode::GU16,  hwc2_eink_refresh_mode_t::HWC2_EINK_GU16_MODE},
    {IEinkMode::CLEAR, hwc2_eink_refresh_mode_t::HWC2_EINK_CLEAR_MODE},
    {IEinkMode::GC4L,  hwc2_eink_refresh_mode_t::HWC2_EINK_GC4L_MODE},
    {IEinkMode::GCC16, hwc2_eink_refresh_mode_t::HWC2_EINK_GCC16_MODE},
    {IEinkMode::RECT,  hwc2_eink_refresh_mode_t::HWC2_EINK_RECT_MODE},
};

namespace android {

class DisplayConfigHal {

public:

    DisplayConfigHal();
    ~DisplayConfigHal();

    static DisplayConfigHal* getInstance();
    void setRefreshMode(int mode, bool handwrite = false);
    void setBuffer(int fd);
    int updateDamage(const Rect& r);
    int forceGlobalRefresh(bool rightNow);
    int setHandwrittenArea(const Rect& r);

private:
    sp<IDisplayConfig> mDisplayConfig;

};

};//namespace android



