#pragma once

#include "AWKeymasterLogger.h"
#include <keymaster/android_keymaster_messages.h>

namespace aw {
namespace hardware {
namespace keymaster {

class AWTACommunicator {
  public:
    AWTACommunicator(AWKeymasterLogger* logger,
                     int32_t message_version = ::keymaster::kDefaultMessageVersion);
    ~AWTACommunicator();
    int LoadTa(const uint8_t* TA_UUID);
    int InvokeTaCommand(int commandID, const ::keymaster::Serializable& req,
                        ::keymaster::KeymasterResponse* rsp);
    int InvokeTaCommand(int commandID, ::keymaster::KeymasterResponse* rsp);

  private:
    void* pTeeMutex;
    void* pTeeContext;
    void* pTeeSession;
    AWKeymasterLogger* logger_;
    int32_t MessageVersion_;
};
}  // namespace keymaster
}  // namespace hardware
}  // namespace aw
