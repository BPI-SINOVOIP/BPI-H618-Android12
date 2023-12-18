/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <keymaster/km_openssl/ecdh_operation.h>

#include <keymaster/km_openssl/ec_key.h>
#include <keymaster/km_openssl/openssl_err.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include <keymaster/logger.h>
#include <openssl/err.h>
#include <vector>

namespace keymaster {

keymaster_error_t EcdhOperation::Begin(const AuthorizationSet& /*input_params*/,
                                       AuthorizationSet* /*output_params*/) {
    auto rc = GenerateRandom(reinterpret_cast<uint8_t*>(&operation_handle_),
                             (size_t)sizeof(operation_handle_));
    if (rc != KM_ERROR_OK) {
        return rc;
    }
    return KM_ERROR_OK;
}

keymaster_error_t EcdhOperation::Update(const AuthorizationSet& /*additional_params*/,
                                        const Buffer& /*input*/,
                                        AuthorizationSet* /*output_params*/, Buffer* /*output*/,
                                        size_t* /*input_consumed*/) {
    return KM_ERROR_OK;
}

keymaster_error_t EcdhOperation::Finish(const AuthorizationSet& /*additional_params*/,
                                        const Buffer& input, const Buffer& /*signature*/,
                                        AuthorizationSet* /*output_params*/, Buffer* output) {
    const unsigned char* encodedPublicKey = input.begin();
    EVP_PKEY* pkeyRaw = d2i_PUBKEY(nullptr, &encodedPublicKey, input.available_read());
    if (pkeyRaw == nullptr) {
        LOG_E("Error decoding key", 0);
        return TranslateLastOpenSslError();
    }
    auto pkey = EVP_PKEY_Ptr(pkeyRaw);

    auto ctx = EVP_PKEY_CTX_Ptr(EVP_PKEY_CTX_new(ecdh_key_.get(), nullptr));
    if (ctx.get() == nullptr) {
        LOG_E("Memory allocation failed", 0);
        return TranslateLastOpenSslError();
    }
    if (EVP_PKEY_derive_init(ctx.get()) != 1) {
        LOG_E("Context initialization failed", 0);
        return TranslateLastOpenSslError();
    }
    if (EVP_PKEY_derive_set_peer(ctx.get(), pkey.get()) != 1) {
        LOG_E("Error setting peer key", 0);
        return TranslateLastOpenSslError();
    }
    size_t sharedSecretLen = 0;
    if (EVP_PKEY_derive(ctx.get(), nullptr, &sharedSecretLen) != 1) {
        LOG_E("Error deriving key", 0);
        return TranslateLastOpenSslError();
    }
    if (!output->reserve(sharedSecretLen)) {
        LOG_E("Error reserving data in output buffer", 0);
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    if (EVP_PKEY_derive(ctx.get(), output->peek_write(), &sharedSecretLen) != 1) {
        LOG_E("Error deriving key", 0);
        return TranslateLastOpenSslError();
    }
    output->advance_write(sharedSecretLen);

    return KM_ERROR_OK;
}

OperationPtr EcdhOperationFactory::CreateOperation(Key&& key,
                                                   const AuthorizationSet& /*begin_params*/,
                                                   keymaster_error_t* error) {
    const EcKey& ecdh_key = static_cast<EcKey&>(key);

    EVP_PKEY_Ptr pkey(EVP_PKEY_new());
    if (pkey.get() == nullptr) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return nullptr;
    }
    if (!ecdh_key.InternalToEvp(pkey.get())) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }

    *error = KM_ERROR_OK;
    auto op = new (std::nothrow)
        EcdhOperation(move(key.hw_enforced_move()), move(key.sw_enforced_move()), pkey.release());
    if (!op) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return nullptr;
    }
    return OperationPtr(op);
}

}  // namespace keymaster
