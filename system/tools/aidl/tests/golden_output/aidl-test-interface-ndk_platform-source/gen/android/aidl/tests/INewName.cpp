#include <android/binder_parcel_utils.h>
#include <aidl/android/aidl/tests/BpNewName.h>
#include <aidl/android/aidl/tests/BnNewName.h>
#include <aidl/android/aidl/tests/INewName.h>

namespace aidl {
namespace android {
namespace aidl {
namespace tests {
static binder_status_t _aidl_onTransact(AIBinder* _aidl_binder, transaction_code_t _aidl_code, const AParcel* _aidl_in, AParcel* _aidl_out) {
  (void)_aidl_in;
  (void)_aidl_out;
  binder_status_t _aidl_ret_status = STATUS_UNKNOWN_TRANSACTION;
  std::shared_ptr<BnNewName> _aidl_impl = std::static_pointer_cast<BnNewName>(::ndk::ICInterface::asInterface(_aidl_binder));
  switch (_aidl_code) {
    case (FIRST_CALL_TRANSACTION + 0 /*RealName*/): {
      std::string _aidl_return;

      ::ndk::ScopedAStatus _aidl_status = _aidl_impl->RealName(&_aidl_return);
      _aidl_ret_status = AParcel_writeStatusHeader(_aidl_out, _aidl_status.get());
      if (_aidl_ret_status != STATUS_OK) break;

      if (!AStatus_isOk(_aidl_status.get())) break;

      _aidl_ret_status = ::ndk::AParcel_writeString(_aidl_out, _aidl_return);
      if (_aidl_ret_status != STATUS_OK) break;

      break;
    }
  }
  return _aidl_ret_status;
}

static AIBinder_Class* _g_aidl_clazz = ::ndk::ICInterface::defineClass(INewName::descriptor, _aidl_onTransact);

BpNewName::BpNewName(const ::ndk::SpAIBinder& binder) : BpCInterface(binder) {}
BpNewName::~BpNewName() {}

::ndk::ScopedAStatus BpNewName::RealName(std::string* _aidl_return) {
  binder_status_t _aidl_ret_status = STATUS_OK;
  ::ndk::ScopedAStatus _aidl_status;
  ::ndk::ScopedAParcel _aidl_in;
  ::ndk::ScopedAParcel _aidl_out;

  _aidl_ret_status = AIBinder_prepareTransaction(asBinder().get(), _aidl_in.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AIBinder_transact(
    asBinder().get(),
    (FIRST_CALL_TRANSACTION + 0 /*RealName*/),
    _aidl_in.getR(),
    _aidl_out.getR(),
    0
    #ifdef BINDER_STABILITY_SUPPORT
    | FLAG_PRIVATE_LOCAL
    #endif  // BINDER_STABILITY_SUPPORT
    );
  if (_aidl_ret_status == STATUS_UNKNOWN_TRANSACTION && INewName::getDefaultImpl()) {
    _aidl_status = INewName::getDefaultImpl()->RealName(_aidl_return);
    goto _aidl_status_return;
  }
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_ret_status = AParcel_readStatusHeader(_aidl_out.get(), _aidl_status.getR());
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  if (!AStatus_isOk(_aidl_status.get())) goto _aidl_status_return;
  _aidl_ret_status = ::ndk::AParcel_readString(_aidl_out.get(), _aidl_return);
  if (_aidl_ret_status != STATUS_OK) goto _aidl_error;

  _aidl_error:
  _aidl_status.set(AStatus_fromStatus(_aidl_ret_status));
  _aidl_status_return:
  return _aidl_status;
}
// Source for BnNewName
BnNewName::BnNewName() {}
BnNewName::~BnNewName() {}
::ndk::SpAIBinder BnNewName::createBinder() {
  AIBinder* binder = AIBinder_new(_g_aidl_clazz, static_cast<void*>(this));
  #ifdef BINDER_STABILITY_SUPPORT
  AIBinder_markCompilationUnitStability(binder);
  #endif  // BINDER_STABILITY_SUPPORT
  return ::ndk::SpAIBinder(binder);
}
// Source for INewName
const char* INewName::descriptor = "android.aidl.tests.IOldName";
INewName::INewName() {}
INewName::~INewName() {}


std::shared_ptr<INewName> INewName::fromBinder(const ::ndk::SpAIBinder& binder) {
  if (!AIBinder_associateClass(binder.get(), _g_aidl_clazz)) { return nullptr; }
  std::shared_ptr<::ndk::ICInterface> interface = ::ndk::ICInterface::asInterface(binder.get());
  if (interface) {
    return std::static_pointer_cast<INewName>(interface);
  }
  return ::ndk::SharedRefBase::make<BpNewName>(binder);
}

binder_status_t INewName::writeToParcel(AParcel* parcel, const std::shared_ptr<INewName>& instance) {
  return AParcel_writeStrongBinder(parcel, instance ? instance->asBinder().get() : nullptr);
}
binder_status_t INewName::readFromParcel(const AParcel* parcel, std::shared_ptr<INewName>* instance) {
  ::ndk::SpAIBinder binder;
  binder_status_t status = AParcel_readStrongBinder(parcel, binder.getR());
  if (status != STATUS_OK) return status;
  *instance = INewName::fromBinder(binder);
  return STATUS_OK;
}
bool INewName::setDefaultImpl(const std::shared_ptr<INewName>& impl) {
  // Only one user of this interface can use this function
  // at a time. This is a heuristic to detect if two different
  // users in the same process use this function.
  assert(!INewName::default_impl);
  if (impl) {
    INewName::default_impl = impl;
    return true;
  }
  return false;
}
const std::shared_ptr<INewName>& INewName::getDefaultImpl() {
  return INewName::default_impl;
}
std::shared_ptr<INewName> INewName::default_impl = nullptr;
::ndk::ScopedAStatus INewNameDefault::RealName(std::string* /*_aidl_return*/) {
  ::ndk::ScopedAStatus _aidl_status;
  _aidl_status.set(AStatus_fromStatus(STATUS_UNKNOWN_TRANSACTION));
  return _aidl_status;
}
::ndk::SpAIBinder INewNameDefault::asBinder() {
  return ::ndk::SpAIBinder();
}
bool INewNameDefault::isRemote() {
  return false;
}
}  // namespace tests
}  // namespace aidl
}  // namespace android
}  // namespace aidl
