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

#define LOG_TAG "android.hardware.keymaster@4.0-impl-aw"

#include "AW_Keymaster4Device.h"

#include <android/log.h>
#include <log/log.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/android_keymaster_messages.h>
#include <keymaster/keymaster_configuration.h>
#include <keymaster/keymaster_context.h>
#include <tee_client_api.h>
#include <hardware/hw_auth_token.h>
#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>

using ::keymaster::ConfigureRequest;
using ::keymaster::ConfigureResponse;
using ::keymaster::AddEntropyRequest;
using ::keymaster::AddEntropyResponse;
using ::keymaster::AttestKeyRequest;
using ::keymaster::AttestKeyResponse;
using ::keymaster::AuthorizationSet;
using ::keymaster::ExportKeyRequest;
using ::keymaster::ExportKeyResponse;
using ::keymaster::GenerateKeyRequest;
using ::keymaster::GenerateKeyResponse;
using ::keymaster::GetKeyCharacteristicsRequest;
using ::keymaster::GetKeyCharacteristicsResponse;
using ::keymaster::ImportKeyRequest;
using ::keymaster::ImportKeyResponse;
using ::keymaster::BeginOperationRequest;
using ::keymaster::BeginOperationResponse;
using ::keymaster::UpdateOperationRequest;
using ::keymaster::UpdateOperationResponse;
using ::keymaster::FinishOperationRequest;
using ::keymaster::FinishOperationResponse;
using ::keymaster::AbortOperationRequest;
using ::keymaster::AbortOperationResponse;
using ::keymaster::UpgradeKeyRequest;
using ::keymaster::UpgradeKeyResponse;
using ::keymaster::DeleteKeyRequest;
using ::keymaster::DeleteKeyResponse;
using ::keymaster::DeleteAllKeysRequest;
using ::keymaster::DeleteAllKeysResponse;
using ::keymaster::ComputeSharedHmacRequest;
using ::keymaster::ComputeSharedHmacResponse;
using ::keymaster::GetHmacSharingParametersResponse;
using ::keymaster::VerifyAuthorizationRequest;
using ::keymaster::VerifyAuthorizationResponse;
using ::keymaster::ImportWrappedKeyRequest;
using ::keymaster::ImportWrappedKeyResponse;

namespace aw {
namespace hardware {
namespace keymaster {
namespace V4_0 {
using ::android::hardware::keymaster::V4_0::Tag;

inline keymaster_tag_t legacy_enum_conversion(const Tag value) {
    return keymaster_tag_t(value);
}

inline keymaster_tag_type_t typeFromTag(const keymaster_tag_t tag) {
    return keymaster_tag_get_type(tag);
}

namespace implementation {
using ::keymaster::KeymasterBlob;

namespace {

inline Tag legacy_enum_conversion(const keymaster_tag_t value) {
    return Tag(value);
}

inline keymaster_purpose_t legacy_enum_conversion(const KeyPurpose value) {
    return static_cast<keymaster_purpose_t>(value);
}

inline keymaster_key_format_t legacy_enum_conversion(const KeyFormat value) {
    return static_cast<keymaster_key_format_t>(value);
}

inline SecurityLevel legacy_enum_conversion(const keymaster_security_level_t value) {
    return static_cast<SecurityLevel>(value);
}

inline hw_authenticator_type_t legacy_enum_conversion(const HardwareAuthenticatorType value) {
    return static_cast<hw_authenticator_type_t>(value);
}

inline ErrorCode legacy_enum_conversion(const keymaster_error_t value) {
    return static_cast<ErrorCode>(value);
}

inline hidl_vec<uint8_t> kmBlob2hidlVec(const keymaster_key_blob_t& blob) {
    hidl_vec<uint8_t> result;
    result.setToExternal(const_cast<unsigned char*>(blob.key_material), blob.key_material_size);
    return result;
}

inline hidl_vec<uint8_t> kmBlob2hidlVec(const keymaster_blob_t& blob) {
    hidl_vec<uint8_t> result;
    result.setToExternal(const_cast<unsigned char*>(blob.data), blob.data_length);
    return result;
}

inline hidl_vec<uint8_t> kmBuffer2hidlVec(const ::keymaster::Buffer& buf) {
    hidl_vec<uint8_t> result;
    result.setToExternal(const_cast<unsigned char*>(buf.peek_read()), buf.available_read());
    return result;
}

inline static hidl_vec<hidl_vec<uint8_t>>
kmCertChain2Hidl(const keymaster_cert_chain_t& cert_chain) {
    hidl_vec<hidl_vec<uint8_t>> result;
    if (!cert_chain.entry_count || !cert_chain.entries) return result;

    result.resize(cert_chain.entry_count);
    for (size_t i = 0; i < cert_chain.entry_count; ++i) {
        result[i] = kmBlob2hidlVec(cert_chain.entries[i]);
    }

    return result;
}

static inline hidl_vec<KeyParameter> kmParamSet2Hidl(const keymaster_key_param_set_t& set) {
    hidl_vec<KeyParameter> result;
    if (set.length == 0 || set.params == nullptr) return result;

    result.resize(set.length);
    keymaster_key_param_t* params = set.params;
    for (size_t i = 0; i < set.length; ++i) {
        auto tag = params[i].tag;
        result[i].tag = legacy_enum_conversion(tag);
        switch (typeFromTag(tag)) {
        case KM_ENUM:
        case KM_ENUM_REP:
            result[i].f.integer = params[i].enumerated;
            break;
        case KM_UINT:
        case KM_UINT_REP:
            result[i].f.integer = params[i].integer;
            break;
        case KM_ULONG:
        case KM_ULONG_REP:
            result[i].f.longInteger = params[i].long_integer;
            break;
        case KM_DATE:
            result[i].f.dateTime = params[i].date_time;
            break;
        case KM_BOOL:
            result[i].f.boolValue = params[i].boolean;
            break;
        case KM_BIGNUM:
        case KM_BYTES:
            result[i].blob.setToExternal(const_cast<unsigned char*>(params[i].blob.data),
                                         params[i].blob.data_length);
            break;
        case KM_INVALID:
        default:
            params[i].tag = KM_TAG_INVALID;
            /* just skip */
            break;
        }
    }
    return result;
}

void addClientAndAppData(const hidl_vec<uint8_t>& clientId, const hidl_vec<uint8_t>& appData,
                         ::keymaster::AuthorizationSet* params) {
    params->Clear();
    if (clientId.size()) {
        params->push_back(::keymaster::TAG_APPLICATION_ID, clientId.data(), clientId.size());
    }
    if (appData.size()) {
        params->push_back(::keymaster::TAG_APPLICATION_DATA, appData.data(), appData.size());
    }
}

static const uint8_t KeyMaster_v4_UUID[16] = {0x7B, 0x01, 0x3D, 0x66, 0x2D, 0x10, 0xE0, 0x4F,
                                              0xC0, 0x86, 0x52, 0x3E, 0x1C, 0x75, 0x48, 0x46};

keymaster_key_param_t genAuthTokenParam(const HardwareAuthToken& authToken) {
    hw_auth_token_t auth_token;
    auth_token.version = HW_AUTH_TOKEN_VERSION;
    auth_token.challenge = authToken.challenge;
    auth_token.authenticator_id = authToken.authenticatorId;
    auth_token.authenticator_type = legacy_enum_conversion(authToken.authenticatorType);
    auth_token.authenticator_type = ::keymaster::hton(auth_token.authenticator_type);
    auth_token.timestamp = ::keymaster::hton(authToken.timestamp);
    auth_token.user_id = authToken.userId;
    memcpy(auth_token.hmac, authToken.mac.data(), authToken.mac.size());
    return Authorization(::keymaster::TAG_AUTH_TOKEN, &auth_token, sizeof(auth_token));
}
}  // anonymous namespace

int AndroidKeymaster4Device_AW::SetUpTa() {
    if (ta_forward_ == nullptr) ta_forward_ = new AWTACommunicator(logger_);
    return ta_forward_->LoadTa(&KeyMaster_v4_UUID[0]);
}

AndroidKeymaster4Device_AW::AndroidKeymaster4Device_AW(std::string logTag)
    : default_logger_(logTag) {
    logger_ = &default_logger_;
    ta_forward_ = nullptr;
}

AndroidKeymaster4Device_AW::~AndroidKeymaster4Device_AW() {
    delete ta_forward_;
}

Return<void> AndroidKeymaster4Device_AW::getHardwareInfo(getHardwareInfo_cb _hidl_cb) {
    _hidl_cb(android::hardware::keymaster::V4_0::SecurityLevel::TRUSTED_ENVIRONMENT,
             "AllWiner-KeymasterDevice", "AllWinner");
    return Void();
}

Return<void>
AndroidKeymaster4Device_AW::getHmacSharingParameters(getHmacSharingParameters_cb _hidl_cb) {
    GetHmacSharingParametersResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_GET_HMAC_SHARING_PARAMETERS, &response);

    ::android::hardware::keymaster::V4_0::HmacSharingParameters params;
    params.seed.setToExternal(const_cast<uint8_t*>(response.params.seed.data),
                              response.params.seed.data_length);
    static_assert(sizeof(response.params.nonce) == params.nonce.size(), "Nonce sizes don't match");
    memcpy(params.nonce.data(), response.params.nonce, params.nonce.size());
    _hidl_cb(legacy_enum_conversion(response.error), params);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::computeSharedHmac(
    const hidl_vec<::android::hardware::keymaster::V4_0::HmacSharingParameters>& params,
    computeSharedHmac_cb _hidl_cb) {
    ComputeSharedHmacRequest request(::keymaster::kDefaultMessageVersion);
    request.params_array.params_array = new ::keymaster::HmacSharingParameters[params.size()];
    request.params_array.num_params = params.size();
    for (size_t i = 0; i < params.size(); ++i) {
        request.params_array.params_array[i].seed = {params[i].seed.data(), params[i].seed.size()};
        static_assert(sizeof(request.params_array.params_array[i].nonce) ==
                          decltype(params[i].nonce)::size(),
                      "Nonce sizes don't match");
        memcpy(request.params_array.params_array[i].nonce, params[i].nonce.data(),
               params[i].nonce.size());
    }

    ComputeSharedHmacResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_COMPUTE_SHARED_HMAC, request, &response);
    hidl_vec<uint8_t> sharing_check;
    if (response.error == KM_ERROR_OK) sharing_check = kmBlob2hidlVec(response.sharing_check);

    _hidl_cb(legacy_enum_conversion(response.error), sharing_check);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::verifyAuthorization(
    uint64_t challenge, const hidl_vec<KeyParameter>& parametersToVerify,
    const ::android::hardware::keymaster::V4_0::HardwareAuthToken& authToken,
    verifyAuthorization_cb _hidl_cb) {

    VerifyAuthorizationRequest request(::keymaster::kDefaultMessageVersion);
    request.challenge = challenge;
    request.parameters_to_verify.Reinitialize(KmParamSet(parametersToVerify));
    request.auth_token.challenge = authToken.challenge;
    request.auth_token.user_id = authToken.userId;
    request.auth_token.authenticator_id = authToken.authenticatorId;
    request.auth_token.authenticator_type = legacy_enum_conversion(authToken.authenticatorType);
    request.auth_token.timestamp = authToken.timestamp;
    KeymasterBlob mac(authToken.mac.data(), authToken.mac.size());
    request.auth_token.mac = mac;

    VerifyAuthorizationResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_VERIFY_AUTHORIZATION, request, &response);

    ::android::hardware::keymaster::V4_0::VerificationToken token;
    token.challenge = response.token.challenge;
    token.timestamp = response.token.timestamp;
    token.parametersVerified = kmParamSet2Hidl(response.token.parameters_verified);
    token.securityLevel = legacy_enum_conversion(response.token.security_level);
    token.mac = kmBlob2hidlVec(response.token.mac);

    _hidl_cb(legacy_enum_conversion(response.error), token);

    return Void();
}

Return<ErrorCode> AndroidKeymaster4Device_AW::addRngEntropy(const hidl_vec<uint8_t>& data) {
    if (data.size() == 0) return ErrorCode::OK;
    AddEntropyRequest request(::keymaster::kDefaultMessageVersion);
    request.random_data.Reinitialize(data.data(), data.size());

    AddEntropyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_ADD_RNG_ENTROPY, request, &response);

    return legacy_enum_conversion(response.error);
}

Return<void> AndroidKeymaster4Device_AW::generateKey(const hidl_vec<KeyParameter>& keyParams,
                                                     generateKey_cb _hidl_cb) {
    GenerateKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.key_description.Reinitialize(KmParamSet(keyParams));

    GenerateKeyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_GENERATE_KEY, request, &response);

    KeyCharacteristics resultCharacteristics;
    hidl_vec<uint8_t> resultKeyBlob;
    if (response.error == KM_ERROR_OK) {
        resultKeyBlob = kmBlob2hidlVec(response.key_blob);
        resultCharacteristics.hardwareEnforced = kmParamSet2Hidl(response.enforced);
        resultCharacteristics.softwareEnforced = kmParamSet2Hidl(response.unenforced);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultKeyBlob, resultCharacteristics);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::getKeyCharacteristics(const hidl_vec<uint8_t>& keyBlob,
                                                               const hidl_vec<uint8_t>& clientId,
                                                               const hidl_vec<uint8_t>& appData,
                                                               getKeyCharacteristics_cb _hidl_cb) {
    GetKeyCharacteristicsRequest request(::keymaster::kDefaultMessageVersion);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());
    addClientAndAppData(clientId, appData, &request.additional_params);

    GetKeyCharacteristicsResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_GET_KEY_CHARACTERISTICS, request, &response);

    KeyCharacteristics resultCharacteristics;
    if (response.error == KM_ERROR_OK) {
        resultCharacteristics.hardwareEnforced = kmParamSet2Hidl(response.enforced);
        resultCharacteristics.softwareEnforced = kmParamSet2Hidl(response.unenforced);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultCharacteristics);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::importKey(const hidl_vec<KeyParameter>& params,
                                                   KeyFormat keyFormat,
                                                   const hidl_vec<uint8_t>& keyData,
                                                   importKey_cb _hidl_cb) {
    ImportKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.key_description.Reinitialize(KmParamSet(params));
    request.key_format = legacy_enum_conversion(keyFormat);
    //request.SetKeyMaterial(keyData.data(), keyData.size());
    request.key_data = ::keymaster::KeymasterKeyBlob(keyData.data(), keyData.size());

    ImportKeyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_IMPORT_KEY, request, &response);

    KeyCharacteristics resultCharacteristics;
    hidl_vec<uint8_t> resultKeyBlob;
    if (response.error == KM_ERROR_OK) {
        resultKeyBlob = kmBlob2hidlVec(response.key_blob);
        resultCharacteristics.hardwareEnforced = kmParamSet2Hidl(response.enforced);
        resultCharacteristics.softwareEnforced = kmParamSet2Hidl(response.unenforced);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultKeyBlob, resultCharacteristics);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::importWrappedKey(
    const hidl_vec<uint8_t>& wrappedKeyData, const hidl_vec<uint8_t>& wrappingKeyBlob,
    const hidl_vec<uint8_t>& maskingKey, const hidl_vec<KeyParameter>& unwrappingParams,
    uint64_t passwordSid, uint64_t biometricSid, importWrappedKey_cb _hidl_cb) {

    ImportWrappedKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.SetWrappedMaterial(wrappedKeyData.data(), wrappedKeyData.size());
    request.SetWrappingMaterial(wrappingKeyBlob.data(), wrappingKeyBlob.size());
    request.SetMaskingKeyMaterial(maskingKey.data(), maskingKey.size());
    request.additional_params.Reinitialize(KmParamSet(unwrappingParams));
    request.password_sid = passwordSid;
    request.biometric_sid = biometricSid;

    ImportWrappedKeyResponse response;
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_IMPORT_WRAPPED_KEY, request, &response);

    KeyCharacteristics resultCharacteristics;
    hidl_vec<uint8_t> resultKeyBlob;
    if (response.error == KM_ERROR_OK) {
        resultKeyBlob = kmBlob2hidlVec(response.key_blob);
        resultCharacteristics.hardwareEnforced = kmParamSet2Hidl(response.enforced);
        resultCharacteristics.softwareEnforced = kmParamSet2Hidl(response.unenforced);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultKeyBlob, resultCharacteristics);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::exportKey(KeyFormat exportFormat,
                                                   const hidl_vec<uint8_t>& keyBlob,
                                                   const hidl_vec<uint8_t>& clientId,
                                                   const hidl_vec<uint8_t>& appData,
                                                   exportKey_cb _hidl_cb) {
    ExportKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.key_format = legacy_enum_conversion(exportFormat);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());
    addClientAndAppData(clientId, appData, &request.additional_params);

    ExportKeyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_EXPORT_KEY, request, &response);

    hidl_vec<uint8_t> resultKeyBlob;
    if (response.error == KM_ERROR_OK) {
        resultKeyBlob.setToExternal(response.key_data, response.key_data_length);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultKeyBlob);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::attestKey(const hidl_vec<uint8_t>& keyToAttest,
                                                   const hidl_vec<KeyParameter>& attestParams,
                                                   attestKey_cb _hidl_cb) {
    AttestKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.SetKeyMaterial(keyToAttest.data(), keyToAttest.size());
    request.attest_params.Reinitialize(KmParamSet(attestParams));

    AttestKeyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_ATTEST_KEY, request, &response);

    hidl_vec<hidl_vec<uint8_t>> resultCertChain;
    if (response.error == KM_ERROR_OK) {
        resultCertChain = kmCertChain2Hidl(response.certificate_chain);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultCertChain);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::upgradeKey(const hidl_vec<uint8_t>& keyBlobToUpgrade,
                                                    const hidl_vec<KeyParameter>& upgradeParams,
                                                    upgradeKey_cb _hidl_cb) {
    // There's nothing to be done to upgrade software key blobs.  Further, the software
    // implementation never returns ErrorCode::KEY_REQUIRES_UPGRADE, so this should never be called.
    UpgradeKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.SetKeyMaterial(keyBlobToUpgrade.data(), keyBlobToUpgrade.size());
    request.upgrade_params.Reinitialize(KmParamSet(upgradeParams));

    UpgradeKeyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_UPGRADE_KEY, request, &response);

    if (response.error == KM_ERROR_OK) {
        _hidl_cb(ErrorCode::OK, kmBlob2hidlVec(response.upgraded_key));
    } else {
        _hidl_cb(legacy_enum_conversion(response.error), hidl_vec<uint8_t>());
    }
    return Void();
}

Return<ErrorCode> AndroidKeymaster4Device_AW::deleteKey(const hidl_vec<uint8_t>& keyBlob) {
    // There's nothing to be done to delete software key blobs.
    DeleteKeyRequest request(::keymaster::kDefaultMessageVersion);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());

    DeleteKeyResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_DELETE_KEY, request, &response);

    return legacy_enum_conversion(response.error);
}

Return<ErrorCode> AndroidKeymaster4Device_AW::deleteAllKeys() {
    // There's nothing to be done to delete software key blobs.
    DeleteAllKeysRequest request(::keymaster::kDefaultMessageVersion);
    DeleteAllKeysResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_DELETE_ALL_KEYS, request, &response);

    return legacy_enum_conversion(response.error);
}

Return<ErrorCode> AndroidKeymaster4Device_AW::destroyAttestationIds() {
    return ErrorCode::UNIMPLEMENTED;
}

Return<void> AndroidKeymaster4Device_AW::begin(KeyPurpose purpose, const hidl_vec<uint8_t>& key,
                                               const hidl_vec<KeyParameter>& inParams,
                                               const HardwareAuthToken& authToken,
                                               begin_cb _hidl_cb) {

    BeginOperationRequest request(::keymaster::kDefaultMessageVersion);
    request.purpose = legacy_enum_conversion(purpose);
    request.SetKeyMaterial(key.data(), key.size());
    request.additional_params.Reinitialize(KmParamSet(inParams));
    request.additional_params.push_back(genAuthTokenParam(authToken));

    BeginOperationResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_BEGIN, request, &response);

    hidl_vec<KeyParameter> resultParams;
    if (response.error == KM_ERROR_OK) {
        resultParams = kmParamSet2Hidl(response.output_params);
    }

    _hidl_cb(legacy_enum_conversion(response.error), resultParams, response.op_handle);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::update(uint64_t operationHandle,
                                                const hidl_vec<KeyParameter>& inParams,
                                                const hidl_vec<uint8_t>& input,
                                                const HardwareAuthToken& authToken,
                                                const VerificationToken& /* verificationToken */,
                                                update_cb _hidl_cb) {
    UpdateOperationRequest request(::keymaster::kDefaultMessageVersion);
    request.op_handle = operationHandle;
    request.input.Reinitialize(input.data(), input.size());
    request.additional_params.Reinitialize(KmParamSet(inParams));
    request.additional_params.push_back(genAuthTokenParam(authToken));

    UpdateOperationResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_UPDATE, request, &response);

    uint32_t resultConsumed = 0;
    hidl_vec<KeyParameter> resultParams;
    hidl_vec<uint8_t> resultBlob;
    if (response.error == KM_ERROR_OK) {
        resultConsumed = response.input_consumed;
        resultParams = kmParamSet2Hidl(response.output_params);
        resultBlob = kmBuffer2hidlVec(response.output);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultConsumed, resultParams, resultBlob);
    return Void();
}

Return<void> AndroidKeymaster4Device_AW::finish(uint64_t operationHandle,
                                                const hidl_vec<KeyParameter>& inParams,
                                                const hidl_vec<uint8_t>& input,
                                                const hidl_vec<uint8_t>& signature,
                                                const HardwareAuthToken& authToken,
                                                const VerificationToken& /* verificationToken */,
                                                finish_cb _hidl_cb) {
    FinishOperationRequest request(::keymaster::kDefaultMessageVersion);
    request.op_handle = operationHandle;
    request.input.Reinitialize(input.data(), input.size());
    request.signature.Reinitialize(signature.data(), signature.size());
    request.additional_params.Reinitialize(KmParamSet(inParams));
    request.additional_params.push_back(genAuthTokenParam(authToken));

    FinishOperationResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_FINISH, request, &response);

    hidl_vec<KeyParameter> resultParams;
    hidl_vec<uint8_t> resultBlob;
    if (response.error == KM_ERROR_OK) {
        resultParams = kmParamSet2Hidl(response.output_params);
        resultBlob = kmBuffer2hidlVec(response.output);
    }
    _hidl_cb(legacy_enum_conversion(response.error), resultParams, resultBlob);
    return Void();
}

Return<ErrorCode> AndroidKeymaster4Device_AW::abort(uint64_t operationHandle) {
    AbortOperationRequest request(::keymaster::kDefaultMessageVersion);
    request.op_handle = operationHandle;

    AbortOperationResponse response(::keymaster::kDefaultMessageVersion);
    ta_forward_->InvokeTaCommand(MSG_KEYMASTER_ABORT, request, &response);

    return legacy_enum_conversion(response.error);
}

void AndroidKeymaster4Device_AW::setLogger(aw::hardware::keymaster::AWKeymasterLogger* logger) {
    logger_->log("switch logger");
    logger_ = logger;
}

}  // namespace implementation

KmParamSet::KmParamSet(const implementation::hidl_vec<implementation::KeyParameter>& keyParams) {
    params = new keymaster_key_param_t[keyParams.size()];
    length = keyParams.size();
    for (size_t i = 0; i < keyParams.size(); ++i) {
        auto tag = legacy_enum_conversion(keyParams[i].tag);
        switch (typeFromTag(tag)) {
        case KM_ENUM:
        case KM_ENUM_REP:
            params[i] = keymaster_param_enum(tag, keyParams[i].f.integer);
            break;
        case KM_UINT:
        case KM_UINT_REP:
            params[i] = keymaster_param_int(tag, keyParams[i].f.integer);
            break;
        case KM_ULONG:
        case KM_ULONG_REP:
            params[i] = keymaster_param_long(tag, keyParams[i].f.longInteger);
            break;
        case KM_DATE:
            params[i] = keymaster_param_date(tag, keyParams[i].f.dateTime);
            break;
        case KM_BOOL:
            if (keyParams[i].f.boolValue)
                params[i] = keymaster_param_bool(tag);
            else
                params[i].tag = KM_TAG_INVALID;
            break;
        case KM_BIGNUM:
        case KM_BYTES:
            params[i] = keymaster_param_blob(tag, &keyParams[i].blob[0], keyParams[i].blob.size());
            break;
        case KM_INVALID:
        default:
            params[i].tag = KM_TAG_INVALID;
            /* just skip */
            break;
        }
    }
}
KmParamSet::KmParamSet(KmParamSet&& other) : keymaster_key_param_set_t{other.params, other.length} {
    other.length = 0;
    other.params = nullptr;
}
KmParamSet::~KmParamSet() {
    delete[] params;
}

}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace aw
