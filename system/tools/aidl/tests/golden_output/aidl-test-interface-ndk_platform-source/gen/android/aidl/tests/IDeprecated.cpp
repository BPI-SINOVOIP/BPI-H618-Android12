#include <android/binder_parcel_utils.h>
#include <aidl/android/aidl/tests/BpDeprecated.h>
#include <aidl/android/aidl/tests/BnDeprecated.h>
#include <aidl/android/aidl/tests/IDeprecated.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
static binder_status_t _aidl_onTransact(AIBinder* _aidl_binder, transaction_code_t _aidl_code, const AParcel* _aidl_in, AParcel* _aidl_out) {
  (void)_aidl_in;
  (void)_aidl_out;
  binder_status_t _aidl_ret_status = STATUS_UNKNOWN_TRANSACTION;
  (void)_aidl_binder;
  (void)_aidl_code;
  return _aidl_ret_status;
}

static AIBinder_Class* _g_aidl_clazz = ::ndk::ICInterface::defineClass(IDeprecated::descriptor, _aidl_onTransact);

#pragma clang diagnostic pop
BpDeprecated::BpDeprecated(const ::ndk::SpAIBinder& binder) : BpCInterface(binder) {}
BpDeprecated::~BpDeprecated() {}

// Source for BnDeprecated
BnDeprecated::BnDeprecated() {}
BnDeprecated::~BnDeprecated() {}
::ndk::SpAIBinder BnDeprecated::createBinder() {
  AIBinder* binder = AIBinder_new(_g_aidl_clazz, static_cast<void*>(this));
  #ifdef BINDER_STABILITY_SUPPORT
  AIBinder_markCompilationUnitStability(binder);
  #endif  // BINDER_STABILITY_SUPPORT
  return ::ndk::SpAIBinder(binder);
}
// Source for IDeprecated
const char* IDeprecated::descriptor = "android.aidl.tests.IDeprecated";
IDeprecated::IDeprecated() {}
IDeprecated::~IDeprecated() {}


std::shared_ptr<IDeprecated> IDeprecated::fromBinder(const ::ndk::SpAIBinder& binder) {
  if (!AIBinder_associateClass(binder.get(), _g_aidl_clazz)) { return nullptr; }
  std::shared_ptr<::ndk::ICInterface> interface = ::ndk::ICInterface::asInterface(binder.get());
  if (interface) {
    return std::static_pointer_cast<IDeprecated>(interface);
  }
  return ::ndk::SharedRefBase::make<BpDeprecated>(binder);
}

binder_status_t IDeprecated::writeToParcel(AParcel* parcel, const std::shared_ptr<IDeprecated>& instance) {
  return AParcel_writeStrongBinder(parcel, instance ? instance->asBinder().get() : nullptr);
}
binder_status_t IDeprecated::readFromParcel(const AParcel* parcel, std::shared_ptr<IDeprecated>* instance) {
  ::ndk::SpAIBinder binder;
  binder_status_t status = AParcel_readStrongBinder(parcel, binder.getR());
  if (status != STATUS_OK) return status;
  *instance = IDeprecated::fromBinder(binder);
  return STATUS_OK;
}
bool IDeprecated::setDefaultImpl(const std::shared_ptr<IDeprecated>& impl) {
  // Only one user of this interface can use this function
  // at a time. This is a heuristic to detect if two different
  // users in the same process use this function.
  assert(!IDeprecated::default_impl);
  if (impl) {
    IDeprecated::default_impl = impl;
    return true;
  }
  return false;
}
const std::shared_ptr<IDeprecated>& IDeprecated::getDefaultImpl() {
  return IDeprecated::default_impl;
}
std::shared_ptr<IDeprecated> IDeprecated::default_impl = nullptr;
::ndk::SpAIBinder IDeprecatedDefault::asBinder() {
  return ::ndk::SpAIBinder();
}
bool IDeprecatedDefault::isRemote() {
  return false;
}
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
