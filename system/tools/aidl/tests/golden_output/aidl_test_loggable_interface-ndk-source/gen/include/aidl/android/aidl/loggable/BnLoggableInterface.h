#pragma once

#include "aidl/android/aidl/loggable/ILoggableInterface.h"

#include <android/binder_ibinder.h>

namespace aidl {
namespace android {
namespace aidl {
namespace loggable {
class BnLoggableInterface : public ::ndk::BnCInterface<ILoggableInterface> {
public:
  BnLoggableInterface();
  virtual ~BnLoggableInterface();
  struct TransactionLog {
    double duration_ms;
    std::string interface_name;
    std::string method_name;
    const void* proxy_address;
    const void* stub_address;
    std::vector<std::pair<std::string, std::string>> input_args;
    std::vector<std::pair<std::string, std::string>> output_args;
    std::string result;
    std::string exception_message;
    int32_t exception_code;
    int32_t transaction_error;
    int32_t service_specific_error_code;
  };
  static std::function<void(const TransactionLog&)> logFunc;
protected:
  ::ndk::SpAIBinder createBinder() override;
private:
};
}  // namespace loggable
}  // namespace aidl
}  // namespace android
}  // namespace aidl
