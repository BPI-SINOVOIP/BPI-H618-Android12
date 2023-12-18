
#include <cutils/properties.h>

#include "debug.h"
#include "utils.h"
#include "output_policy.h"
#include "hardware/sunxi_display2.h"

const int kDisconnected = 0;
const int kConnected = 1;
const char* policyName[3] = {"unknow", "single", "dual"};

DefaultOutputPolicy::DefaultOutputPolicy(
        const std::vector<OutputDeviceInfo>& perfectMapping)
{
    initOutputPolicy();
    initPerfectMapping(perfectMapping);
}

std::string DefaultOutputPolicy::getDebugName() const
{
    std::string debugName("DefaultOutputPolicy");
    debugName + ": " + policyName[mPolicy];
    return debugName;
}

void DefaultOutputPolicy::initOutputPolicy()
{
    int policy = property_get_int32(DISPLAY_POLICY_PROPERTY, eDualOutput);
    if (policy != eSingleOutput && policy != eDualOutput) {
        dd_error("unknow display policy config: %d, change to dual display output", policy);
        policy = eDualOutput;
    }
    mPolicy = policy;
    dd_info("display output policy: %s", policyName[policy]);
}

void DefaultOutputPolicy::initPerfectMapping(
        const std::vector<OutputDeviceInfo>& perfectMapping)
{
    for (const auto& item : perfectMapping) {
        int logicalDisplayId = item.logicalId;
        OutputDeviceInfo info = item;
        info.connected = kDisconnected;
        mPerfectMapping.emplace(logicalDisplayId, info);
        dd_info("Display.%d output type: %d", logicalDisplayId, info.type);
    }
}

void DefaultOutputPolicy::updateConnectState(int type, int connected)
{
    dd_debug("updateConnectState: type=%d connected=%d", type, connected);
    std::unique_lock<std::mutex> lock(mLock);

    for (auto& item : mPerfectMapping) {
        OutputDeviceInfo& info = item.second;
        info.connected = getConnectState(info.type);
        dd_debug("updated: type=%d connected=%d", info.type, info.connected);
    }

    /*
     * On first trigger, wait enought time for primary display setup.
     * In case the switch uevent of primary display is later than the
     * external display.
     */

    if (type == -1 && connected == -1
            && mPerfectMapping[ePrimaryDisplayId].connected != kConnected) {
        usleep(1000 * 100);
        OutputDeviceInfo& info = mPerfectMapping[ePrimaryDisplayId];
        info.connected = getConnectState(info.type);
        dd_debug("updateConnectState on primary after 100 ms, connected = %d", info.connected);
    }

    rebuildDisplayMappingLocked();
}

static inline OutputDeviceInfo generateDeviceInfo(int id, int type, int enable)
{
    OutputDeviceInfo info{ id, type, enable, 0 };
    return info;
}

void DefaultOutputPolicy::buildDualOutput()
{
    const auto& primary  = mPerfectMapping[ePrimaryDisplayId ];
    const auto& external = mPerfectMapping[eExternalDisplayId];

    OutputDeviceInfo infos[2];

    if (primary.connected == kConnected) {
        infos[0] = generateDeviceInfo(ePrimaryDisplayId, primary.type, 1);
        infos[1] = generateDeviceInfo(eExternalDisplayId, external.type, external.connected);
    } else {
        if (external.connected == kConnected) {
            infos[0] = generateDeviceInfo(ePrimaryDisplayId, external.type, 1);
            infos[1] = generateDeviceInfo(eExternalDisplayId, primary.type, 0);
        } else {
            infos[0] = generateDeviceInfo(ePrimaryDisplayId, primary.type, 1);
            infos[1] = generateDeviceInfo(eExternalDisplayId, external.type, 0);
        }
    }

    mCurrentMapping.emplace(ePrimaryDisplayId,  infos[0]);
    mCurrentMapping.emplace(eExternalDisplayId, infos[1]);
}

void DefaultOutputPolicy::buildSingleOutput()
{
    const auto& primary  = mPerfectMapping[ePrimaryDisplayId ];
    const auto& external = mPerfectMapping[eExternalDisplayId];

    OutputDeviceInfo infos[2];

    if (primary.connected == kConnected) {
        infos[0] = generateDeviceInfo(ePrimaryDisplayId, primary.type, 1);
        infos[1] = generateDeviceInfo(eExternalDisplayId, external.type, 0);
    } else if (external.connected == kConnected) {
        infos[0] = generateDeviceInfo(ePrimaryDisplayId, external.type, 1);
        infos[1] = generateDeviceInfo(eExternalDisplayId, primary.type, 0);
    } else {
        infos[0] = generateDeviceInfo(ePrimaryDisplayId, primary.type, 1);
        infos[1] = generateDeviceInfo(eExternalDisplayId, external.type, 0);
    }

    mCurrentMapping.emplace(ePrimaryDisplayId,  infos[0]);
    mCurrentMapping.emplace(eExternalDisplayId, infos[1]);
}

void DefaultOutputPolicy::rebuildDisplayMappingLocked()
{
    mCurrentMapping.clear();

    if (mPerfectMapping.size() == 1) {
        const auto& primary  = mPerfectMapping[ePrimaryDisplayId];
        OutputDeviceInfo info{ePrimaryDisplayId, primary.type, 1, 0};
        mCurrentMapping.emplace(ePrimaryDisplayId, info);
        return;
    }

    switch (mPolicy) {
        case eDualOutput:
            buildDualOutput();
            break;
        case eSingleOutput:
            buildSingleOutput();
            break;
        default:
            dd_error("unknow output policy");
            break;
    }
}

void DefaultOutputPolicy::getOutputDeviceMapping(std::vector<OutputDeviceInfo>& out)
{
    std::unique_lock<std::mutex> lock(mLock);

    out.clear();
    for (auto& item : mCurrentMapping) {
        out.push_back(item.second);
    }
}

int DefaultOutputPolicy::getConnectState(int type)
{

    int connected = 0;

    switch (type) {
        case DISP_OUTPUT_TYPE_HDMI:
        case DISP_OUTPUT_TYPE_TV:
            connected = platformGetDeviceConnectState(type);
            break;
        case DISP_OUTPUT_TYPE_LCD:
            connected = 1;
            break;
        default:
            connected = -1;
    }
    if (connected < 0) {
        dd_error("getConnectState failed, connected = -1");
        connected = 0;
    }
    return connected;
}

std::unique_ptr<OutputPolicy> createOutputPolicy(
        const std::vector<OutputDeviceInfo>& perfectMapping)
{
    std::unique_ptr<OutputPolicy> policy =
        std::make_unique<DefaultOutputPolicy>(perfectMapping);
    return policy;
}

