/*
 * Copyright 2015 The Android Open Source Project
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

#pragma once

#include <hardware/keymaster_defs.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>

namespace keymaster {

class Buffer;
class RandomSource;

// Define the formats this code knows about.  Note that "format" here implies both structure and KEK
// derivation and encryption algorithm, though the KEK derivation and encryption is performed prior
// to serialization.
enum AuthEncryptedBlobFormat : uint8_t {
    AES_OCB = 0,
    AES_GCM_WITH_SW_ENFORCED = 1,
};

struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;

    MoveOnly& operator=(MoveOnly&&) = default;
    void operator=(const MoveOnly&) = delete;
};

struct EncryptedKey : private MoveOnly {
    AuthEncryptedBlobFormat format;
    KeymasterKeyBlob ciphertext;
    Buffer nonce;
    Buffer tag;
};

struct DeserializedKey : private MoveOnly {
    EncryptedKey encrypted_key;
    AuthorizationSet hw_enforced;
    AuthorizationSet sw_enforced;
};

/**
 * Encrypt the provided plaintext with format `format`, using the provided authorization lists and
 * master_key to derive the key encryption key.
 */
EncryptedKey EncryptKey(const KeymasterKeyBlob& plaintext, AuthEncryptedBlobFormat format,
                        const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
                        const AuthorizationSet& hidden, const KeymasterKeyBlob& master_key,
                        const RandomSource& random, keymaster_error_t* error);

/**
 * Serialize `encrypted_key` (which contains necessary nonce & tag information),
 * along with the associated authorization data into a blob.
 */
KeymasterKeyBlob SerializeAuthEncryptedBlob(const EncryptedKey& encrypted_key,
                                            const AuthorizationSet& hw_enforced,
                                            const AuthorizationSet& sw_enforced,
                                            keymaster_error_t* error);

/**
 * Deserialize a blob, retrieving the key ciphertext, decryption parameters and associated
 * authorization lists.
 */
DeserializedKey DeserializeAuthEncryptedBlob(const KeymasterKeyBlob& key_blob,
                                             keymaster_error_t* error);

/**
 * Decrypt key material from the Deserialized data in `key'.
 */
KeymasterKeyBlob DecryptKey(const DeserializedKey& key, const AuthorizationSet& hidden,
                            const KeymasterKeyBlob& master_key, keymaster_error_t* error);

}  // namespace keymaster
