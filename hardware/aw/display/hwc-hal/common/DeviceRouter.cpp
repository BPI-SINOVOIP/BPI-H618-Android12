
#include "Debug.h"
#include "DeviceRouter.h"

using namespace sunxi;

DeviceRouter* DeviceRouter::mInstance = new DeviceRouter();

void DeviceRouter::onDeviceChanged(std::vector<DeviceMapping>& maps)
{
    // We intend to to wait all composition had complete before switching device,
    // In case we assign the layer on device A, but submit it on device B.
    std::unique_lock<std::mutex> lock(mDeviceLock);
    mCondition.wait(lock,
            [this]{ return mSwitchDisableMask == 0; });

    mCurrentMapping.clear();
    mCurrentMapping.swap(maps);

    // update device on each active connection
    for (auto& item : mCurrentMapping) {
        if (mConnections.count(item.LogicDisplayId)) {
            auto& connection = mConnections[item.LogicDisplayId];
            connection->onDeviceChanged(item.HardwareDevice);
        }
    }
}

void DeviceRouter::switchEnable(int32_t id, bool enable)
{
    std::unique_lock<std::mutex> lock(mDeviceLock);

    int32_t mask = 1 << id;
    if (enable) {
        mSwitchDisableMask &= ~mask;
        mCondition.notify_all();
    } else {
        mSwitchDisableMask |= mask;
    }
}

void DeviceRouter::registerHotplugCallback(HotplugCallback cb)
{
    if (mHotplugCallback != nullptr) {
        DLOGW("HotplugCallback has already been registered, skip!");
        return;
    }
    mHotplugCallback = cb;
}

void DeviceRouter::performHotplug(int32_t id, bool connected)
{
    std::unique_lock<std::mutex> lock(mDeviceLock);
    mCondition.wait(lock,
            [this]{ return mSwitchDisableMask == 0; });

    // set connection as abandoned,
    // make sure it will not commit anything after that.
    if (mConnections.count(id)) {
        auto& connection = mConnections[id];
        connection->abandoned();
    }

    mHotplugCallback(id, connected);
}

std::shared_ptr<DeviceConnection> DeviceRouter::connect(int32_t id)
{
    return createDeviceConnection(id);
}

void DeviceRouter::disconnect(int32_t id)
{
    mConnections.erase(id);

    std::vector<DeviceMapping>::iterator it = std::find_if(
            mCurrentMapping.begin(),
            mCurrentMapping.end(),
            [&](const auto& dev) { return dev.LogicDisplayId == id; });

    if (it != mCurrentMapping.end()) {
        mCurrentMapping.erase(it);
    }
}

std::shared_ptr<DeviceConnection> DeviceRouter::createDeviceConnection(int32_t id)
{
    if (mConnections.count(id)) {
        mConnections.erase(id);
        DLOGW("Erase duplicate connection (Display id: %d)", id);
    }

    std::vector<DeviceMapping>::iterator it = std::find_if(
            mCurrentMapping.begin(),
            mCurrentMapping.end(),
            [&](const auto& dev) { return dev.LogicDisplayId == id; });

    if (it != mCurrentMapping.end()) {
        std::shared_ptr<DeviceBase> device = (*it).HardwareDevice;
        std::shared_ptr<DeviceConnection> connection =
            std::make_shared<DeviceConnection>(*this, id, device);
        mConnections.emplace(id, connection);
        return connection;
    }

    DLOGE("Can't find out display.%d", id);
    return nullptr;
}

// ------------------------ DeviceConnection -----------------------//

DeviceConnection::DeviceConnection(
        DeviceRouter& r, int32_t id, std::shared_ptr<DeviceBase>& dev)
  : mDisplayId(id),
    mAbandoned(false),
    mDeviceRouter(r),
    mVsyncEnabled(false),
    mVsyncCallback(nullptr),
    mRefreshCallback(nullptr),
    mDevice(dev)
{

}

DeviceConnection::~DeviceConnection() { }

const DeviceBase::Config& DeviceConnection::getDefaultConfig()
{
    std::shared_lock<std::shared_mutex> lock(mSharedMutex);
    return mDevice->getDefaultConfig();
}

int32_t DeviceConnection::getRefreshRate()
{
    std::shared_lock<std::shared_mutex> lock(mSharedMutex);
    return mDevice->getRefreshRate();
}

int32_t DeviceConnection::setPowerMode(int32_t mode)
{
    std::shared_lock<std::shared_mutex> lock(mSharedMutex);
    return mDevice->setPowerMode(mode);
}

int32_t DeviceConnection::setVsyncEnabled(bool enabled)
{
    mVsyncEnabled = enabled;
    mDevice->setVsyncEnabled(enabled);
    return 0;
}

void DeviceConnection::validate(CompositionContext *ctx)
{
    // Disable device swithing untill composition finish
    setSwitchingEnable(false);
    if (mAbandoned) {
        setSwitchingEnable(true);
        return;
    }

    std::shared_lock<std::shared_mutex> lock(mSharedMutex);
    mDevice->prepare(ctx);
}

void DeviceConnection::present(CompositionContext *ctx)
{
    std::shared_lock<std::shared_mutex> lock(mSharedMutex);
    mDevice->commit(ctx);

    // Enable device switching
    setSwitchingEnable(true);
}

void DeviceConnection::updateEventListenerLocked()
{
    mDevice->setEventListener(
            std::weak_ptr<EventListener>(shared_from_this()));
}

void DeviceConnection::setVsyncCallback(VsyncCallback cb)
{
    std::shared_lock<std::shared_mutex> lock(mSharedMutex);
    mVsyncCallback = cb;
    updateEventListenerLocked();
}

void DeviceConnection::setRefreshCallback(RefreshCallback cb)
{
    mRefreshCallback = cb;
}

void DeviceConnection::setSwitchingEnable(bool enable)
{
    mDeviceRouter.switchEnable(mDisplayId, enable);
}

void DeviceConnection::onDeviceChanged(std::shared_ptr<DeviceBase>& dev) {
    std::unique_lock<std::shared_mutex> lock(mSharedMutex);

    if (dev == nullptr) {
        DLOGE("device should not be nullptr!");
        assert(dev != nullptr);
    }

    mDevice = dev;
    updateEventListenerLocked();

    //mDevice->setVsyncEnabled(mVsyncEnabled);
    //TODO
    //if (mRefreshCallback != nullptr) {
    //    mRefreshCallback();
    //}
}

void DeviceConnection::onVsync(int64_t timestamp)
{
    if (mVsyncCallback)
        mVsyncCallback(timestamp);
}

void DeviceConnection::dump(std::string& out)
{
    std::unique_lock<std::shared_mutex> lock(mSharedMutex);
    mDevice->dump(out);
}

