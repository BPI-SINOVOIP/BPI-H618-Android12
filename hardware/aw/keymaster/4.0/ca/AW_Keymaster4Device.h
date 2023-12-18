/*
 **
 ** Copyright 2016, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef HIDL_aw_hardware_keymaster_V4_0_AndroidKeymaster4Device_AW_H_
#define HIDL_aw_hardware_keymaster_V4_0_AndroidKeymaster4Device_AW_H_

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <keymaster/android_keymaster_messages.h>
#include <AW_KM_utility/AWKeymasterLogger.h>
#include <AW_KM_utility/AWTACommunicator.h>


namespace aw {
namespace hardware {
namespace keymaster {
namespace V4_0 {

class KmParamSet : public keymaster_key_param_set_t {
  public:
    KmParamSet(const android::hardware::hidl_vec<android::hardware::keymaster::V4_0::KeyParameter>&
                   keyParams);
    KmParamSet(KmParamSet&& other);
    KmParamSet(const KmParamSet&) = delete;
    ~KmParamSet();
};

enum {
    MSG_KEYMASTER_V2_INITIALIZE = 0x201,
    MSG_KEYMASTER_V2_TERMINATE,
    MSG_KEYMASTER_V2_CONFIGURE = 0x210,
    MSG_KEYMASTER_ADD_RNG_ENTROPY = 0x212,
    MSG_KEYMASTER_GENERATE_KEY = 0x214,
    MSG_KEYMASTER_GET_KEY_CHARACTERISTICS = 0x216,
    MSG_KEYMASTER_EXPORT_KEY = 0x21D,
    MSG_KEYMASTER_ATTEST_KEY = 0x21E,
    MSG_KEYMASTER_UPGRADE_KEY = 0x220,
    MSG_KEYMASTER_DELETE_KEY = 0x222,
    MSG_KEYMASTER_DELETE_ALL_KEYS = 0x224,
    MSG_KEYMASTER_BEGIN = 0x226,
    MSG_KEYMASTER_UPDATE = 0x228,
    MSG_KEYMASTER_FINISH = 0x22a,
    MSG_KEYMASTER_ABORT = 0x22c,
    MSG_KEYMASTER_IMPORT_KEY = 0x230,
    MSG_KEYMASTER_DESTROY_ATTESTATION_IDS = 0x231,
    MSG_KEYMASTER_IMPORT_WRAPPED_KEY = 0x232,
    MSG_KEYMASTER_GET_HARDWARE_INFO = 0x233,
    MSG_KEYMASTER_GET_HMAC_SHARING_PARAMETERS = 0x234,
    MSG_KEYMASTER_COMPUTE_SHARED_HMAC = 0x235,
    MSG_KEYMASTER_VERIFY_AUTHORIZATION = 0x236,
};

namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::keymaster::V4_0::ErrorCode;
using ::android::hardware::keymaster::V4_0::HardwareAuthenticatorType;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::HmacSharingParameters;
using ::android::hardware::keymaster::V4_0::IKeymasterDevice;
using ::android::hardware::keymaster::V4_0::KeyCharacteristics;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::KeyParameter;
using ::android::hardware::keymaster::V4_0::KeyPurpose;
using ::android::hardware::keymaster::V4_0::SecurityLevel;
using ::android::hardware::keymaster::V4_0::Tag;
using ::android::hardware::keymaster::V4_0::VerificationToken;

class AndroidKeymaster4Device_AW : public IKeymasterDevice {
  public:
    AndroidKeymaster4Device_AW(std::string logTag = "AW_Keymsater_4.0");
    virtual ~AndroidKeymaster4Device_AW();

    Return<void> getHardwareInfo(getHardwareInfo_cb _hidl_cb) override;
    Return<void> getHmacSharingParameters(getHmacSharingParameters_cb _hidl_cb) override;
    Return<void> computeSharedHmac(const hidl_vec<HmacSharingParameters>& params,
                                   computeSharedHmac_cb) override;
    Return<void> verifyAuthorization(uint64_t challenge,
                                     const hidl_vec<KeyParameter>& parametersToVerify,
                                     const HardwareAuthToken& authToken,
                                     verifyAuthorization_cb _hidl_cb) override;
    Return<ErrorCode> addRngEntropy(const hidl_vec<uint8_t>& data) override;
    Return<void> generateKey(const hidl_vec<KeyParameter>& keyParams,
                             generateKey_cb _hidl_cb) override;
    Return<void> getKeyCharacteristics(const hidl_vec<uint8_t>& keyBlob,
                                       const hidl_vec<uint8_t>& clientId,
                                       const hidl_vec<uint8_t>& appData,
                                       getKeyCharacteristics_cb _hidl_cb) override;
    Return<void> importKey(const hidl_vec<KeyParameter>& params, KeyFormat keyFormat,
                           const hidl_vec<uint8_t>& keyData, importKey_cb _hidl_cb) override;
    Return<void> importWrappedKey(const hidl_vec<uint8_t>& wrappedKeyData,
                                  const hidl_vec<uint8_t>& wrappingKeyBlob,
                                  const hidl_vec<uint8_t>& maskingKey,
                                  const hidl_vec<KeyParameter>& unwrappingParams,
                                  uint64_t passwordSid, uint64_t biometricSid,
                                  importWrappedKey_cb _hidl_cb) override;
    Return<void> exportKey(KeyFormat exportFormat, const hidl_vec<uint8_t>& keyBlob,
                           const hidl_vec<uint8_t>& clientId, const hidl_vec<uint8_t>& appData,
                           exportKey_cb _hidl_cb) override;
    Return<void> attestKey(const hidl_vec<uint8_t>& keyToAttest,
                           const hidl_vec<KeyParameter>& attestParams,
                           attestKey_cb _hidl_cb) override;
    Return<void> upgradeKey(const hidl_vec<uint8_t>& keyBlobToUpgrade,
                            const hidl_vec<KeyParameter>& upgradeParams,
                            upgradeKey_cb _hidl_cb) override;
    Return<ErrorCode> deleteKey(const hidl_vec<uint8_t>& keyBlob) override;
    Return<ErrorCode> deleteAllKeys() override;
    Return<ErrorCode> destroyAttestationIds() override;
    Return<void> begin(KeyPurpose purpose, const hidl_vec<uint8_t>& key,
                       const hidl_vec<KeyParameter>& inParams, const HardwareAuthToken& authToken,
                       begin_cb _hidl_cb) override;
    Return<void> update(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken, update_cb _hidl_cb) override;
    Return<void> finish(uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
                        const hidl_vec<uint8_t>& input, const hidl_vec<uint8_t>& signature,
                        const HardwareAuthToken& authToken,
                        const VerificationToken& verificationToken, finish_cb _hidl_cb) override;
    Return<ErrorCode> abort(uint64_t operationHandle) override;
    int SetUpTa();
    void setLogger(::aw::hardware::keymaster::AWKeymasterLogger* logger);

  protected:
    ::aw::hardware::keymaster::AWTACommunicator* ta_forward_;
    ::aw::hardware::keymaster::AWKeymasterLogger* logger_;
    ::aw::hardware::keymaster::AWKeymasterLogger default_logger_;
};

}  // namespace implementation
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace aw

#endif  // HIDL_aw_hardware_keymaster_V4_0_AndroidKeymaster4Device_AW_H_
