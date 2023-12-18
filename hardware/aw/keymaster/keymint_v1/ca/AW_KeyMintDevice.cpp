/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.security.keymint-impl"
#include <android-base/logging.h>

#include "include/AW_KeyMintDevice.h"

#include <aidl/android/hardware/security/keymint/ErrorCode.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/contexts/pure_soft_keymaster_context.h>
#include <keymaster/keymaster_configuration.h>

#include "include/AW_KeyMintOperation.h"
#include "include/KeyMintUtils.h"

#include "include/AW_KM_TA_commands.h"

namespace aidl::android::hardware::security::keymint {

using namespace keymaster;  // NOLINT(google-build-using-namespace)

using km_utils::authToken2AidlVec;
using km_utils::kmBlob2vector;
using km_utils::kmError2ScopedAStatus;
using km_utils::kmParam2Aidl;
using km_utils::KmParamSet;
using km_utils::kmParamSet2Aidl;
using km_utils::legacy_enum_conversion;
using secureclock::TimeStampToken;

namespace {
static constexpr int32_t MessageVersion_ =
    ::keymaster::MessageVersion(::keymaster::KmVersion::KEYMINT_1);

KeyCharacteristics convertAuthSet(SecurityLevel securityLevel,
                                  const keymaster::AuthorizationSet& authorizations) {
    KeyCharacteristics retval{securityLevel, {}};
    std::transform(authorizations.begin(), authorizations.end(),
                   std::back_inserter(retval.authorizations), kmParam2Aidl);
    return retval;
}
vector<KeyCharacteristics> convertKeyCharacteristics(SecurityLevel keyMintSecurityLevel,
                                                     const AuthorizationSet& sw_enforced,
                                                     const AuthorizationSet& hw_enforced,
                                                     bool include_keystore_enforced = true) {
    KeyCharacteristics keyMintEnforced = convertAuthSet(keyMintSecurityLevel, hw_enforced);
    KeyCharacteristics keystoreEnforced = convertAuthSet(SecurityLevel::KEYSTORE, sw_enforced);

    vector<KeyCharacteristics> retval;
    retval.reserve(2);

    if (!keyMintEnforced.authorizations.empty()) retval.push_back(std::move(keyMintEnforced));
    if (include_keystore_enforced && !keystoreEnforced.authorizations.empty()) {
        retval.push_back(std::move(keystoreEnforced));
    }

    return retval;
}

Certificate convertCertificate(const keymaster_blob_t& cert) {
    return {std::vector<uint8_t>(cert.data, cert.data + cert.data_length)};
}

vector<Certificate> convertCertificateChain(const CertificateChain& chain) {
    vector<Certificate> retval;
    retval.reserve(chain.entry_count);
    std::transform(chain.begin(), chain.end(), std::back_inserter(retval), convertCertificate);
    return retval;
}

void addClientAndAppData(const std::vector<uint8_t>& appId, const std::vector<uint8_t>& appData,
                         ::keymaster::AuthorizationSet* params) {
    params->Clear();
    if (appId.size()) {
        params->push_back(::keymaster::TAG_APPLICATION_ID, appId.data(), appId.size());
    }
    if (appData.size()) {
        params->push_back(::keymaster::TAG_APPLICATION_DATA, appData.data(), appData.size());
    }
}

}  // namespace

AW_KeyMintDevice::AW_KeyMintDevice(AWTACommunicator* ta, AWKeymasterLogger* log)
    : ta_forward_(ta), logger_(log) {
    securityLevel_ = SecurityLevel::TRUSTED_ENVIRONMENT;
}

AW_KeyMintDevice::~AW_KeyMintDevice() {}

ScopedAStatus AW_KeyMintDevice::getHardwareInfo(KeyMintHardwareInfo* info) {
    info->versionNumber = 1;
    info->securityLevel = securityLevel_;
    info->keyMintName = "AWKeyMintDevice";
    info->keyMintAuthorName = "Allwinner";
    info->timestampTokenRequired = false;
    return ScopedAStatus::ok();
}

ScopedAStatus AW_KeyMintDevice::addRngEntropy(const vector<uint8_t>& data) {
    if (data.size() == 0) {
        return ScopedAStatus::ok();
    }

    AddEntropyRequest request(MessageVersion_);
    request.random_data.Reinitialize(data.data(), data.size());

    AddEntropyResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_ADD_RNG_ENTROPY, request,
                                 &response);

    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AW_KeyMintDevice::generateKey(const vector<KeyParameter>& keyParams,
                                            const optional<AttestationKey>& attestationKey,
                                            KeyCreationResult* creationResult) {

    GenerateKeyRequest request(MessageVersion_);
    request.key_description.Reinitialize(KmParamSet(keyParams));
    if (attestationKey) {
        request.attestation_signing_key_blob =
            KeymasterKeyBlob(attestationKey->keyBlob.data(), attestationKey->keyBlob.size());
        request.attest_key_params.Reinitialize(KmParamSet(attestationKey->attestKeyParams));
        request.issuer_subject = KeymasterBlob(attestationKey->issuerSubjectName.data(),
                                               attestationKey->issuerSubjectName.size());
    }

    GenerateKeyResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_GENERATE_KEY, request, &response);

    if (response.error != KM_ERROR_OK) {
        // Note a key difference between this current aidl and previous hal, is
        // that hal returns void where as aidl returns the error status.  If
        // aidl returns error, then aidl will not return any change you may make
        // to the out parameters.  This is quite different from hal where all
        // output variable can be modified due to hal returning void.
        //
        // So the caller need to be aware not to expect aidl functions to clear
        // the output variables for you in case of error.  If you left some
        // wrong data set in the out parameters, they will stay there.
        return kmError2ScopedAStatus(response.error);
    }

    creationResult->keyBlob = kmBlob2vector(response.key_blob);
    creationResult->keyCharacteristics = convertKeyCharacteristics(
        securityLevel_, response.unenforced, response.enforced);
    creationResult->certificateChain = convertCertificateChain(response.certificate_chain);
    return ScopedAStatus::ok();
}

ScopedAStatus AW_KeyMintDevice::importKey(const vector<KeyParameter>& keyParams,
                                          KeyFormat keyFormat, const vector<uint8_t>& keyData,
                                          const optional<AttestationKey>& attestationKey,
                                          KeyCreationResult* creationResult) {

    ImportKeyRequest request(MessageVersion_);
    request.key_description.Reinitialize(KmParamSet(keyParams));
    request.key_format = legacy_enum_conversion(keyFormat);
    request.key_data = KeymasterKeyBlob(keyData.data(), keyData.size());
    if (attestationKey) {
        request.attestation_signing_key_blob =
            KeymasterKeyBlob(attestationKey->keyBlob.data(), attestationKey->keyBlob.size());
        request.attest_key_params.Reinitialize(KmParamSet(attestationKey->attestKeyParams));
        request.issuer_subject = KeymasterBlob(attestationKey->issuerSubjectName.data(),
                                               attestationKey->issuerSubjectName.size());
    }

    ImportKeyResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_IMPORT_KEY, request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    creationResult->keyBlob = kmBlob2vector(response.key_blob);
    creationResult->keyCharacteristics = convertKeyCharacteristics(
        securityLevel_, response.unenforced, response.enforced);
    creationResult->certificateChain = convertCertificateChain(response.certificate_chain);

    return ScopedAStatus::ok();
}

ScopedAStatus AW_KeyMintDevice::importWrappedKey(const vector<uint8_t>& wrappedKeyData,         //
                                                 const vector<uint8_t>& wrappingKeyBlob,        //
                                                 const vector<uint8_t>& maskingKey,             //
                                                 const vector<KeyParameter>& unwrappingParams,  //
                                                 int64_t passwordSid, int64_t biometricSid,     //
                                                 KeyCreationResult* creationResult) {

    ImportWrappedKeyRequest request(MessageVersion_);
    request.SetWrappedMaterial(wrappedKeyData.data(), wrappedKeyData.size());
    request.SetWrappingMaterial(wrappingKeyBlob.data(), wrappingKeyBlob.size());
    request.SetMaskingKeyMaterial(maskingKey.data(), maskingKey.size());
    request.additional_params.Reinitialize(KmParamSet(unwrappingParams));
    request.password_sid = static_cast<uint64_t>(passwordSid);
    request.biometric_sid = static_cast<uint64_t>(biometricSid);

    ImportWrappedKeyResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_IMPORT_WRAPPED_KEY, request,
                                 &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    creationResult->keyBlob = kmBlob2vector(response.key_blob);
    creationResult->keyCharacteristics = convertKeyCharacteristics(
        securityLevel_, response.unenforced, response.enforced);
    creationResult->certificateChain = convertCertificateChain(response.certificate_chain);

    return ScopedAStatus::ok();
}

ScopedAStatus AW_KeyMintDevice::upgradeKey(const vector<uint8_t>& keyBlobToUpgrade,
                                           const vector<KeyParameter>& upgradeParams,
                                           vector<uint8_t>* keyBlob) {

    UpgradeKeyRequest request(MessageVersion_);
    request.SetKeyMaterial(keyBlobToUpgrade.data(), keyBlobToUpgrade.size());
    request.upgrade_params.Reinitialize(KmParamSet(upgradeParams));

    UpgradeKeyResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_UPGRADE_KEY, request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    *keyBlob = kmBlob2vector(response.upgraded_key);
    return ScopedAStatus::ok();
}

ScopedAStatus AW_KeyMintDevice::deleteKey(const vector<uint8_t>& keyBlob) {
    DeleteKeyRequest request(MessageVersion_);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());

    DeleteKeyResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_DELETE_KEY, request, &response);

    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AW_KeyMintDevice::deleteAllKeys() {
    // There's nothing to be done to delete software key blobs.
    DeleteAllKeysRequest request(MessageVersion_);
    DeleteAllKeysResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_DELETE_ALL_KEYS, request,
                                 &response);

    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AW_KeyMintDevice::destroyAttestationIds() {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AW_KeyMintDevice::begin(KeyPurpose purpose, const vector<uint8_t>& keyBlob,
                                      const vector<KeyParameter>& params,
                                      const optional<HardwareAuthToken>& authToken,
                                      BeginResult* result) {

    BeginOperationRequest request(MessageVersion_);
    request.purpose = legacy_enum_conversion(purpose);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());
    request.additional_params.Reinitialize(KmParamSet(params));

    vector<uint8_t> vector_token = authToken2AidlVec(authToken);
    request.additional_params.push_back(
        TAG_AUTH_TOKEN, reinterpret_cast<uint8_t*>(vector_token.data()), vector_token.size());

    BeginOperationResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_BEGIN, request, &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    result->params = kmParamSet2Aidl(response.output_params);
    result->challenge = response.op_handle;
    result->operation =
        ndk::SharedRefBase::make<AW_KeyMintOperation>(ta_forward_, logger_, response.op_handle);
    return ScopedAStatus::ok();
}

ScopedAStatus
AW_KeyMintDevice::deviceLocked(bool passwordOnly,
                               const std::optional<secureclock::TimeStampToken>& timestampToken) {
    DeviceLockedRequest request(MessageVersion_);
    request.passwordOnly = passwordOnly;
    if (timestampToken.has_value()) {
        request.token.challenge = timestampToken->challenge;
        request.token.mac = {timestampToken->mac.data(), timestampToken->mac.size()};
        request.token.timestamp = timestampToken->timestamp.milliSeconds;
    }
    DeviceLockedResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_DEVICE_LOCKED, request,
                                 &response);
    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus AW_KeyMintDevice::earlyBootEnded() {
    EarlyBootEndedResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_EARLY_BOOT_ENDED, &response);
    return kmError2ScopedAStatus(response.error);
}

ScopedAStatus
AW_KeyMintDevice::convertStorageKeyToEphemeral(const std::vector<uint8_t>& /* storageKeyBlob */,
                                               std::vector<uint8_t>* /* ephemeralKeyBlob */) {
    return kmError2ScopedAStatus(KM_ERROR_UNIMPLEMENTED);
}

ScopedAStatus AW_KeyMintDevice::getKeyCharacteristics(
    const std::vector<uint8_t>& keyBlob, const std::vector<uint8_t>& appId,
    const std::vector<uint8_t>& appData, std::vector<KeyCharacteristics>* keyCharacteristics) {
    GetKeyCharacteristicsRequest request(MessageVersion_);
    request.SetKeyMaterial(keyBlob.data(), keyBlob.size());
    addClientAndAppData(appId, appData, &request.additional_params);

    GetKeyCharacteristicsResponse response(MessageVersion_);
    ta_forward_->InvokeTaCommand(AW_KM_TA_COMMANDS::MSG_KEYMASTER_GET_KEY_CHARACTERISTICS, request,
                                 &response);

    if (response.error != KM_ERROR_OK) {
        return kmError2ScopedAStatus(response.error);
    }

    AuthorizationSet emptySet;
    *keyCharacteristics =
        convertKeyCharacteristics(securityLevel_, response.unenforced, response.enforced,
                                  /* include_keystore_enforced = */ false);

    return ScopedAStatus::ok();
}

int AW_KeyMintDevice::SetUpTa(AWTACommunicator* ta_forward) {

    static const uint8_t KeyMint_v1_UUID[16] = {0x58, 0x9C, 0xFF, 0x75, 0xD0, 0x92, 0xF3, 0x4F,
                                                0xE7, 0x87, 0x21, 0x76, 0xA1, 0x87, 0x3C, 0xD4};
    return ta_forward->LoadTa(KeyMint_v1_UUID);
}

}  // namespace aidl::android::hardware::security::keymint
