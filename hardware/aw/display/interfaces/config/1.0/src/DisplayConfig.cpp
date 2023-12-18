
#include <utils/Log.h>
#include "DisplayConfig.h"

namespace vendor {
namespace display {
namespace config {
namespace V1_0 {
namespace implementation {

#undef  LOG_TAG
#define LOG_TAG "DisplayConfig"
#define DEBUG(x, ...) \
    ALOGD("%s: " x, __PRETTY_FUNCTION__, ##__VA_ARGS__)

using ::vendor::display::config::V1_0::DisplayPortType;
using ::vendor::display::config::V1_0::LayerMode;
using ::vendor::display::config::V1_0::PixelFormat;
using ::vendor::display::config::V1_0::Dataspace;
using ::vendor::display::config::V1_0::AspectRatio;
using ::vendor::display::config::V1_0::ScreenMargin;
using ::vendor::display::config::V1_0::EnhanceItem;
using ::vendor::display::config::V1_0::SNRInfo;

Return<int32_t> DisplayConfig::setDisplayArgs(int32_t display,
        int32_t cmd1, int32_t cmd2, int32_t data)
{
    return mConfigImpl->setDisplayArgs(display, cmd1, cmd2, data);
}

Return<DisplayPortType> DisplayConfig::getDisplayPortType(int32_t display)
{
    int type = mConfigImpl->getPortType(display);
    return DisplayPortType(type);
}

Return<int32_t> DisplayConfig::setDisplayPortType(int32_t display, DisplayPortType type)
{
    return mConfigImpl->setPortType(display, static_cast<int32_t>(type));
}

Return<void> DisplayConfig::getActiveMode(int32_t display, getActiveMode_cb _hidl_cb)
{
    int mode = mConfigImpl->getMode(display);
    if (mode < 0) {
        _hidl_cb(-1, 0);
        DEBUG("getMode error=%d", mode);
        return Void();
    }
    _hidl_cb(0, mode);
    return Void();
}

Return<int32_t> DisplayConfig::setActiveMode(int32_t display, uint32_t mode)
{
    return mConfigImpl->setMode(display, mode);
}

Return<void> DisplayConfig::getSupportedModes(int32_t display, getSupportedModes_cb _hidl_cb)
{
    std::vector<int> supportModes;
    int ret = mConfigImpl->getSupportedModes(display, supportModes);

    DEBUG("display[%d] ret=%d, supported modes count(%zd)",
            display, ret, supportModes.size());

    if (!ret && supportModes.size()) {
        std::vector<uint32_t> tmp;
        for (size_t i = 0; i < supportModes.size(); i++) {
            tmp.push_back(supportModes[i]);
        }
        hidl_vec<uint32_t> result;
        result.setToExternal(tmp.data(), tmp.size());
        _hidl_cb(result);
    } else {
        // Abort message: _hidl_cb not called, but must be called once.
        hidl_vec<uint32_t> result;
        _hidl_cb(result);
    }

    return Void();
}

Return<bool> DisplayConfig::supported3D(int32_t display)
{
    int supported = mConfigImpl->supported3D(display);
    return (supported == 1);
}

Return<LayerMode> DisplayConfig::get3DLayerMode(int32_t display)
{
    int lm = mConfigImpl->get3DLayerMode(display);
    return LayerMode(lm);
}

Return<int32_t> DisplayConfig::set3DLayerMode(int32_t display, LayerMode mode)
{
    return mConfigImpl->set3DLayerMode(display, static_cast<int>(mode));
}

Return<void> DisplayConfig::getSupportedPixelFormats(int32_t display,
        getSupportedPixelFormats_cb _hidl_cb)
{
    std::vector<int> supportFormats;
    int ret = mConfigImpl->getSupportedPixelFormats(display, supportFormats);

    DEBUG("display[%d] ret=%d, supported pixel formats count(%zd)",
            display, ret, supportFormats.size());

    if (!ret && supportFormats.size()) {
        std::vector<PixelFormat> tmp;
        for (size_t i = 0; i < supportFormats.size(); i++) {
            tmp.push_back(PixelFormat(supportFormats[i]));
        }
        hidl_vec<PixelFormat> result;
        result.setToExternal(tmp.data(), tmp.size());
        _hidl_cb(result);
    }

    return Void();
}

Return<PixelFormat> DisplayConfig::getPixelFormat(int32_t display)
{
    int format = mConfigImpl->getPixelFormat(display);
    return PixelFormat(format);
}

Return<int32_t> DisplayConfig::setPixelFormat(int32_t display, PixelFormat fmt)
{
    int error = mConfigImpl->setPixelFormat(display, static_cast<int>(fmt));
    return error;
}

Return<Dataspace> DisplayConfig::getCurrentDataspace(int32_t display)
{
    int dataspace = mConfigImpl->getCurrentDataspace(display);
    return Dataspace(dataspace);
}

Return<Dataspace> DisplayConfig::getDataspaceMode(int32_t display)
{
    int mode = mConfigImpl->getDataspaceMode(display);
    return Dataspace(mode);
}

Return<int32_t> DisplayConfig::setDataspaceMode(int32_t display, Dataspace mode)
{
    return mConfigImpl->setDataspaceMode(display, static_cast<int>(mode));
}

Return<int32_t> DisplayConfig::setAspectRatio(int32_t display, AspectRatio ratio)
{
    return mConfigImpl->setAspectRatio(display, static_cast<int>(ratio));
}

Return<AspectRatio> DisplayConfig::getAspectRatio(int32_t display)
{
    int ratio = mConfigImpl->getAspectRatio(display);
    return AspectRatio(ratio);
}

Return<int32_t> DisplayConfig::setScreenMargin(int32_t display, const ScreenMargin& margin)
{
    return mConfigImpl->setMargin(display,
                margin.left, margin.right, margin.top, margin.bottom);
}

Return<void> DisplayConfig::getScreenMargin(int32_t display, getScreenMargin_cb _hidl_cb)
{
    std::vector<int> tmp;
    int ret = mConfigImpl->getMargin(display, tmp);

    ScreenMargin margin;
    if (!ret && tmp.size() == 4) {
        margin.left   = tmp[0];
        margin.right  = tmp[1];
        margin.top    = tmp[2];
        margin.bottom = tmp[3];
    } else {
        margin.left   = 100;
        margin.right  = 100;
        margin.top    = 100;
        margin.bottom = 100;
    };
    _hidl_cb(margin);
    return Void();
}

Return<int32_t> DisplayConfig::getEnhanceComponent(int32_t display, EnhanceItem item)
{
    return mConfigImpl->getEnhanceComponent(display, static_cast<int32_t>(item));
}

Return<int32_t> DisplayConfig::setEnhanceComponent(int32_t display, EnhanceItem item, int32_t value)
{
    return mConfigImpl->setEnhanceComponent(display, static_cast<int32_t>(item), value);
}

Return<bool> DisplayConfig::supportedSNRSetting(int32_t display) {
    return mConfigImpl->supportedSNRSetting(display);
}

Return<void> DisplayConfig::getSNRInfo(int32_t display, getSNRInfo_cb _hidl_cb) {
    SNRInfo info;
    int ret = mConfigImpl->getSNRInfo(display, info);
    if (ret) {
        DEBUG("mConfigImpl->getSNRInfo return error, ret=%d", ret);
        info.mode = SNRFeatureMode::SNR_DISABLE;
        info.y = 0;
        info.u = 0;
        info.v = 0;
    }
    _hidl_cb(ret, info);
    return Void();
}

Return<int32_t> DisplayConfig::setSNRInfo(int32_t display, const ::vendor::display::config::V1_0::SNRInfo& snr) {
    return mConfigImpl->setSNRInfo(display, snr);
}

Return<int32_t> DisplayConfig::configHdcp(bool enable)
{
    return mConfigImpl->configHdcp(enable);
}

Return<::vendor::display::config::V1_0::HdcpLevel> DisplayConfig::getConnectedHdcpLevel() {
    int32_t level = mConfigImpl->getConnectedHdcpLevel();
    return static_cast<::vendor::display::config::V1_0::HdcpLevel>(level);
}

Return<::vendor::display::config::V1_0::HdcpAuthorizedStatus> DisplayConfig::getAuthorizedStatus() {
    int32_t status = mConfigImpl->getAuthorizedStatus();
    return static_cast<::vendor::display::config::V1_0::HdcpAuthorizedStatus>(status);
}

Return<void> DisplayConfig::dumpDebugInfo(dumpDebugInfo_cb _hidl_cb)
{
    std::string buf;
    mConfigImpl->dump(buf);
    hidl_string buf_reply(buf.data(), buf.size());
    _hidl_cb(buf_reply);
    return Void();
}

Return<int32_t> DisplayConfig::getHDMINativeMode(int32_t display)
{
    return mConfigImpl->getHDMINativeMode(display);
}

Return<int32_t> DisplayConfig::getHdmiUserSetting(int32_t display)
{
    return mConfigImpl->getHdmiUserSetting(display);
}

Return<int32_t> DisplayConfig::setEinkMode(int32_t mode)
{
    return mConfigImpl->setEinkMode(mode);
}

Return<int32_t> DisplayConfig::setEinkBufferFd(const hidl_handle& fd)
{
	return mConfigImpl->setEinkBufferFd(fd.getNativeHandle()->data[0]);
}

Return<int32_t> DisplayConfig::updateEinkRegion(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    return mConfigImpl->updateEinkRegion(left, top, right, bottom);
}

Return<int32_t> DisplayConfig::forceEinkRefresh(bool rightNow)
{
    return mConfigImpl->forceEinkRefresh(rightNow);
}

Return<int32_t> DisplayConfig::setEinkUpdateArea(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    return mConfigImpl->setEinkUpdateArea(left, top, right, bottom);
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace config
}  // namespace display
}  // namespace vendor
