
#pragma once

#include <vendor/display/config/1.0/IDisplayConfig.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include "DisplayConfigImpl.h"

namespace vendor {
namespace display {
namespace config {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using ::sunxi::DisplayConfigImpl;

struct DisplayConfig : public IDisplayConfig {
    // Methods from ::vendor::display::config::V1_0::IDisplayConfig follow.
    Return<int32_t> setDisplayArgs(int32_t display, int32_t cmd1, int32_t cmd2, int32_t data) override;
    Return<::vendor::display::config::V1_0::DisplayPortType> getDisplayPortType(int32_t display) override;
    Return<int32_t> setDisplayPortType(int32_t display, ::vendor::display::config::V1_0::DisplayPortType type) override;
    Return<void> getActiveMode(int32_t display, getActiveMode_cb _hidl_cb) override;
    Return<int32_t> setActiveMode(int32_t display, uint32_t mode) override;
    Return<void> getSupportedModes(int32_t display, getSupportedModes_cb _hidl_cb) override;
    Return<bool> supported3D(int32_t display) override;
    Return<::vendor::display::config::V1_0::LayerMode> get3DLayerMode(int32_t display) override;
    Return<int32_t> set3DLayerMode(int32_t display, ::vendor::display::config::V1_0::LayerMode mode) override;
    Return<void> getSupportedPixelFormats(int32_t display, getSupportedPixelFormats_cb _hidl_cb) override;
    Return<::vendor::display::config::V1_0::PixelFormat> getPixelFormat(int32_t display) override;
    Return<int32_t> setPixelFormat(int32_t display, ::vendor::display::config::V1_0::PixelFormat fmt) override;
    Return<::vendor::display::config::V1_0::Dataspace> getCurrentDataspace(int32_t display) override;
    Return<::vendor::display::config::V1_0::Dataspace> getDataspaceMode(int32_t display) override;
    Return<int32_t> setDataspaceMode(int32_t display, ::vendor::display::config::V1_0::Dataspace mode) override;
    Return<int32_t> setAspectRatio(int32_t display, ::vendor::display::config::V1_0::AspectRatio ratio) override;
    Return<::vendor::display::config::V1_0::AspectRatio> getAspectRatio(int32_t display) override;
    Return<int32_t> setScreenMargin(int32_t display, const ::vendor::display::config::V1_0::ScreenMargin& margin) override;
    Return<void> getScreenMargin(int32_t display, getScreenMargin_cb _hidl_cb) override;
    Return<int32_t> getEnhanceComponent(int32_t display, ::vendor::display::config::V1_0::EnhanceItem item) override;
    Return<int32_t> setEnhanceComponent(int32_t display, ::vendor::display::config::V1_0::EnhanceItem item, int32_t value) override;
    Return<bool> supportedSNRSetting(int32_t display) override;
    Return<void> getSNRInfo(int32_t display, getSNRInfo_cb _hidl_cb) override;
    Return<int32_t> setSNRInfo(int32_t display, const ::vendor::display::config::V1_0::SNRInfo& snr) override;
    Return<int32_t> configHdcp(bool enable) override;
    Return<::vendor::display::config::V1_0::HdcpLevel> getConnectedHdcpLevel() override;
    Return<::vendor::display::config::V1_0::HdcpAuthorizedStatus> getAuthorizedStatus() override;
    Return<void> dumpDebugInfo(dumpDebugInfo_cb _hidl_cb) override;
    Return<int32_t> getHDMINativeMode(int32_t display) override;
    Return<int32_t> getHdmiUserSetting(int32_t display) override;
    Return<int32_t> setEinkMode(int32_t mode) override;
    Return<int32_t> setEinkBufferFd(const hidl_handle& fd) override;
    Return<int32_t> updateEinkRegion(int32_t left, int32_t top, int32_t right, int32_t bottom) override;
    Return<int32_t> forceEinkRefresh(bool rightNow) override;
    Return<int32_t> setEinkUpdateArea(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    DisplayConfig(sp<DisplayConfigImpl>& impl)
        : mConfigImpl(impl) { }

private:
    sp<DisplayConfigImpl> mConfigImpl;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace display
}  // namespace vendor
