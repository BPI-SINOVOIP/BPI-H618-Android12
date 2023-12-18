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

#include <keymaster/contexts/pure_soft_remote_provisioning_context.h>

#include <algorithm>
#include <assert.h>

#include <keymaster/cppcose/cppcose.h>
#include <keymaster/logger.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/hkdf.h>
#include <openssl/rand.h>

namespace keymaster {

using cppcose::constructCoseSign1;
using cppcose::CoseKey;
using cppcose::ED25519;
using cppcose::EDDSA;
using cppcose::OCTET_KEY_PAIR;
using cppcose::VERIFY;

constexpr uint32_t kMacKeyLength = 32;

PureSoftRemoteProvisioningContext::PureSoftRemoteProvisioningContext() {
    std::tie(devicePrivKey_, bcc_) = GenerateBcc(/*testMode=*/false);
}

PureSoftRemoteProvisioningContext::~PureSoftRemoteProvisioningContext() {}

std::vector<uint8_t>
PureSoftRemoteProvisioningContext::DeriveBytesFromHbk(const std::string& context,
                                                      size_t num_bytes) const {
    // bytevec fakeHbk(32, 0);
    // bytevec result(numBytes);
    // is it safe to revert the call below back to the call above?
    std::vector<uint8_t> fakeHbk(32);
    std::vector<uint8_t> result(num_bytes);

    // TODO: Figure out if HKDF can fail.  It doesn't seem like it should be able to,
    // but the function does return an error code.
    HKDF(result.data(), num_bytes,              //
         EVP_sha256(),                          //
         fakeHbk.data(), fakeHbk.size(),        //
         nullptr /* salt */, 0 /* salt len */,  //
         reinterpret_cast<const uint8_t*>(context.data()), context.size());

    return result;
}

std::unique_ptr<cppbor::Map> PureSoftRemoteProvisioningContext::CreateDeviceInfo() const {
    auto result = std::make_unique<cppbor::Map>(cppbor::Map());

    // The following placeholders show how the DeviceInfo map would be populated.
    // result->add(cppbor::Tstr("brand"), cppbor::Tstr("Google"));
    // result->add(cppbor::Tstr("manufacturer"), cppbor::Tstr("Google"));
    // result->add(cppbor::Tstr("product"), cppbor::Tstr("Fake"));
    // result->add(cppbor::Tstr("model"), cppbor::Tstr("Imaginary"));
    // result->add(cppbor::Tstr("board"), cppbor::Tstr("Chess"));
    // result->add(cppbor::Tstr("vb_state"), cppbor::Tstr("orange"));
    // result->add(cppbor::Tstr("bootloader_state"), cppbor::Tstr("unlocked"));
    // result->add(cppbor::Tstr("os_version"), cppbor::Tstr("SC"));
    // result->add(cppbor::Tstr("system_patch_level"), cppbor::Uint(20210331));
    // result->add(cppbor::Tstr("boot_patch_level"), cppbor::Uint(20210331));
    // result->add(cppbor::Tstr("vendor_patch_level"), cppbor::Uint(20210331));

    result->canonicalize();
    return result;
}

std::pair<std::vector<uint8_t> /* privKey */, cppbor::Array /* BCC */>
PureSoftRemoteProvisioningContext::GenerateBcc(bool testMode) const {
    std::vector<uint8_t> privKey(ED25519_PRIVATE_KEY_LEN);
    std::vector<uint8_t> pubKey(ED25519_PUBLIC_KEY_LEN);

    uint8_t seed[32];  // Length is hard-coded in the BoringCrypto API
    if (testMode) {
        RAND_bytes(seed, sizeof(seed));
    } else {
        auto seed_vector = DeriveBytesFromHbk("Device Key Seed", sizeof(seed));
        std::copy(seed_vector.begin(), seed_vector.end(), seed);
    }
    ED25519_keypair_from_seed(pubKey.data(), privKey.data(), seed);

    auto coseKey = cppbor::Map()
                       .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                       .add(CoseKey::ALGORITHM, EDDSA)
                       .add(CoseKey::CURVE, ED25519)
                       .add(CoseKey::KEY_OPS, VERIFY)
                       .add(CoseKey::PUBKEY_X, pubKey)
                       .canonicalize();
    auto sign1Payload = cppbor::Map()
                            .add(1 /* Issuer */, "Issuer")
                            .add(2 /* Subject */, "Subject")
                            .add(-4670552 /* Subject Pub Key */, coseKey.encode())
                            .add(-4670553 /* Key Usage (little-endian order) */,
                                 std::vector<uint8_t>{0x20} /* keyCertSign = 1<<5 */)
                            .canonicalize()
                            .encode();
    auto coseSign1 = constructCoseSign1(privKey,       /* signing key */
                                        cppbor::Map(), /* extra protected */
                                        sign1Payload, {} /* AAD */);
    assert(coseSign1);

    return {privKey, cppbor::Array().add(std::move(coseKey)).add(coseSign1.moveValue())};
}

std::optional<cppcose::HmacSha256>
PureSoftRemoteProvisioningContext::GenerateHmacSha256(const cppcose::bytevec& input) const {
    auto key = DeriveBytesFromHbk("Key to MAC public keys", kMacKeyLength);
    auto result = cppcose::generateHmacSha256(key, input);
    if (!result) {
        LOG_E("Error signing MAC: %s", result.message().c_str());
        return std::nullopt;
    }
    return *result;
}

}  // namespace keymaster
