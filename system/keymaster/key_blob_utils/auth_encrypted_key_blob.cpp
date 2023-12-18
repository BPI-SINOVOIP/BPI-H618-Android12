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

#include <keymaster/key_blob_utils/auth_encrypted_key_blob.h>

#include <openssl/digest.h>
#include <openssl/evp.h>
#include <openssl/hkdf.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>
#include <keymaster/key_blob_utils/ocb_utils.h>
#include <keymaster/km_openssl/openssl_err.h>
#include <keymaster/logger.h>
#include <keymaster/random_source.h>

namespace keymaster {

namespace {

constexpr uint8_t kAesGcmDescriptor[] = "AES-256-GCM-HKDF-SHA-256, version 1";
constexpr size_t kAesGcmNonceLength = 12;
constexpr size_t kAesGcmTagLength = 16;
constexpr size_t kAes256KeyLength = 256 / 8;

Buffer generate_nonce(const RandomSource& random, size_t size, keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    Buffer nonce;
    if (!nonce.Reinitialize(size)) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return {};
    }

    random.GenerateRandom(nonce.peek_write(), size);
    nonce.advance_write(size);
    return nonce;
}

Buffer BuildAesGcmInfo(const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
                       const AuthorizationSet& hidden, keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    size_t info_len = sizeof(kAesGcmDescriptor) + hidden.SerializedSize() +
                      hw_enforced.SerializedSize() + sw_enforced.SerializedSize();
    Buffer info(info_len);

    info.write(kAesGcmDescriptor, sizeof(kAesGcmDescriptor));
    uint8_t* buf = info.peek_write();
    const uint8_t* end = info.peek_write() + info.available_write();
    buf = hidden.Serialize(buf, end);
    buf = hw_enforced.Serialize(buf, end);
    buf = sw_enforced.Serialize(buf, end);

    if (!buf || buf != end || !info.advance_write(buf - info.peek_write())) {
        LOG_S("Buffer management error", 0);
        *error = KM_ERROR_UNKNOWN_ERROR;
        return {};
    }

    return info;
}

Buffer DeriveAesGcmKeyEncryptionKey(const AuthorizationSet& hw_enforced,
                                    const AuthorizationSet& sw_enforced,
                                    const AuthorizationSet& hidden,
                                    const KeymasterKeyBlob& master_key, keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    Buffer prk(EVP_MAX_MD_SIZE);
    size_t out_len = EVP_MAX_MD_SIZE;
    if (!HKDF_extract(prk.peek_write(), &out_len, EVP_sha256(), master_key.key_material,
                      master_key.key_material_size, nullptr /* salt */, 0 /* salt_len */)) {
        *error = TranslateLastOpenSslError();
        return {};
    }

    Buffer info = BuildAesGcmInfo(hw_enforced, sw_enforced, hidden, error);
    if (KM_ERROR_OK != *error) return {};

    if (!prk.advance_write(out_len) || !prk.available_read() || !info.available_read()) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return {};
    }

    Buffer keyEncryptionKey(kAes256KeyLength);
    if (!HKDF_expand(keyEncryptionKey.peek_write(), keyEncryptionKey.available_write(),  //
                     EVP_sha256(),                                                       //
                     prk.peek_read(), prk.available_read(),                              //
                     info.peek_read(), info.available_read())) {
        *error = TranslateLastOpenSslError();
        return {};
    }

    return keyEncryptionKey;
}

EncryptedKey AesGcmEncryptKey(const AuthorizationSet& hw_enforced,  //
                              const AuthorizationSet& sw_enforced,  //
                              const AuthorizationSet& hidden,       //
                              const KeymasterKeyBlob& master_key,   //
                              const KeymasterKeyBlob& plaintext,    //
                              AuthEncryptedBlobFormat format,       //
                              Buffer nonce,                         //
                              keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    Buffer kek = DeriveAesGcmKeyEncryptionKey(hw_enforced, sw_enforced, hidden, master_key, error);
    if (KM_ERROR_OK != *error) return {};

    bssl::UniquePtr<EVP_CIPHER_CTX> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return {};
    }

    int ciphertext_len = plaintext.size();
    int unused_len = 0;
    EncryptedKey retval;
    retval.format = format;
    retval.ciphertext = KeymasterKeyBlob(ciphertext_len);
    retval.nonce = move(nonce);
    retval.tag = Buffer(kAesGcmTagLength);

    if (!(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr /* engine */, kek.peek_read(),
                             retval.nonce.peek_read()) &&
          EVP_EncryptUpdate(ctx.get(), retval.ciphertext.writable_data(), &ciphertext_len,
                            plaintext.key_material, plaintext.size()) &&
          EVP_EncryptFinal_ex(ctx.get(), retval.ciphertext.writable_data() /* not written to */,
                              &unused_len) &&
          EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kAesGcmTagLength,
                              retval.tag.peek_write()))) {
        *error = TranslateLastOpenSslError();
        return {};
    }

    if (plaintext.size() != static_cast<size_t>(ciphertext_len) || 0 != unused_len ||
        !retval.tag.advance_write(kAesGcmTagLength)) {
        *error = KM_ERROR_UNKNOWN_ERROR;
    }

    return retval;
}

KeymasterKeyBlob AesGcmDecryptKey(const DeserializedKey& key, const AuthorizationSet& hidden,
                                  const KeymasterKeyBlob& master_key, keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    Buffer kek =
        DeriveAesGcmKeyEncryptionKey(key.hw_enforced, key.sw_enforced, hidden, master_key, error);
    if (KM_ERROR_OK != *error) return {};

    bssl::UniquePtr<EVP_CIPHER_CTX> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return {};
    }

    int plaintext_len = key.encrypted_key.ciphertext.size();
    int unused_len = 0;
    KeymasterKeyBlob plaintext(plaintext_len);
    if (!(EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr /* engine */, kek.peek_read(),
                             key.encrypted_key.nonce.peek_read()) &&
          EVP_DecryptUpdate(ctx.get(), plaintext.writable_data(), &plaintext_len,
                            key.encrypted_key.ciphertext.key_material,
                            key.encrypted_key.ciphertext.size()) &&
          EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kAesGcmTagLength,
                              const_cast<uint8_t*>(key.encrypted_key.tag.peek_read())))) {
        *error = TranslateLastOpenSslError();
        return {};
    }

    if (!EVP_DecryptFinal_ex(ctx.get(), plaintext.writable_data() /* not written to */,
                             &unused_len)) {
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return {};
    }

    if (key.encrypted_key.ciphertext.size() != plaintext.size() || 0 != unused_len) {
        *error = KM_ERROR_UNKNOWN_ERROR;
    }

    return plaintext;
}

}  // namespace

KeymasterKeyBlob SerializeAuthEncryptedBlob(const EncryptedKey& encrypted_key,
                                            const AuthorizationSet& hw_enforced,
                                            const AuthorizationSet& sw_enforced,
                                            keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    size_t size = 1 /* version byte */ + encrypted_key.nonce.SerializedSize() +
                  encrypted_key.ciphertext.SerializedSize() + encrypted_key.tag.SerializedSize() +
                  hw_enforced.SerializedSize() + sw_enforced.SerializedSize();

    KeymasterKeyBlob retval;
    if (!retval.Reset(size)) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return {};
    }

    uint8_t* buf = retval.writable_data();
    const uint8_t* end = retval.end();

    *buf++ = encrypted_key.format;
    buf = encrypted_key.nonce.Serialize(buf, end);
    buf = encrypted_key.ciphertext.Serialize(buf, end);
    buf = encrypted_key.tag.Serialize(buf, end);
    buf = hw_enforced.Serialize(buf, end);
    buf = sw_enforced.Serialize(buf, end);
    if (buf != retval.end()) *error = KM_ERROR_UNKNOWN_ERROR;

    return retval;
}

DeserializedKey DeserializeAuthEncryptedBlob(const KeymasterKeyBlob& key_blob,
                                             keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    if (!key_blob.key_material || key_blob.key_material_size == 0) {
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return {};
    }

    const uint8_t* tmp = key_blob.key_material;
    const uint8_t** buf_ptr = &tmp;
    const uint8_t* end = tmp + key_blob.key_material_size;

    if (end <= *buf_ptr) {
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return {};
    }

    DeserializedKey retval;
    retval.encrypted_key.format = static_cast<AuthEncryptedBlobFormat>(*(*buf_ptr)++);
    if (!retval.encrypted_key.nonce.Deserialize(buf_ptr, end) ||       //
        !retval.encrypted_key.ciphertext.Deserialize(buf_ptr, end) ||  //
        !retval.encrypted_key.tag.Deserialize(buf_ptr, end) ||         //
        !retval.hw_enforced.Deserialize(buf_ptr, end) ||               //
        !retval.sw_enforced.Deserialize(buf_ptr, end) ||               //
        *buf_ptr != end) {
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return {};
    }

    switch (retval.encrypted_key.format) {
    case AES_OCB:
        if (retval.encrypted_key.nonce.available_read() != OCB_NONCE_LENGTH ||
            retval.encrypted_key.tag.available_read() != OCB_TAG_LENGTH) {
            *error = KM_ERROR_INVALID_KEY_BLOB;
            return {};
        }
        return retval;

    case AES_GCM_WITH_SW_ENFORCED:
        if (retval.encrypted_key.nonce.available_read() != kAesGcmNonceLength ||
            retval.encrypted_key.tag.available_read() != kAesGcmTagLength) {
            *error = KM_ERROR_INVALID_KEY_BLOB;
            return {};
        }
        return retval;
    }

    *error = KM_ERROR_INVALID_KEY_BLOB;
    return {};
}

EncryptedKey EncryptKey(const KeymasterKeyBlob& plaintext, AuthEncryptedBlobFormat format,
                        const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
                        const AuthorizationSet& hidden, const KeymasterKeyBlob& master_key,
                        const RandomSource& random, keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    switch (format) {
    case AES_OCB: {
        EncryptedKey retval;
        retval.format = format;
        retval.nonce = generate_nonce(random, OCB_NONCE_LENGTH, error);
        retval.tag.Reinitialize(OCB_TAG_LENGTH);
        if (KM_ERROR_OK != *error) return {};
        *error = OcbEncryptKey(hw_enforced, sw_enforced, hidden, master_key, plaintext,
                               retval.nonce, &retval.ciphertext, &retval.tag);
        return retval;
    }

    case AES_GCM_WITH_SW_ENFORCED: {
        auto nonce = generate_nonce(random, kAesGcmNonceLength, error);
        if (KM_ERROR_OK != *error) return {};
        return AesGcmEncryptKey(hw_enforced, sw_enforced, hidden, master_key, plaintext, format,
                                move(nonce), error);
    }
    }

    *error = KM_ERROR_UNKNOWN_ERROR;
    LOG_E("Invalid key blob format %d", format);
    return {};
}

KeymasterKeyBlob DecryptKey(const DeserializedKey& key, const AuthorizationSet& hidden,
                            const KeymasterKeyBlob& master_key, keymaster_error_t* error) {
    if (!error) return {};
    *error = KM_ERROR_OK;

    KeymasterKeyBlob retval;

    switch (key.encrypted_key.format) {
    case AES_OCB:
        *error = OcbDecryptKey(key.hw_enforced, key.sw_enforced, hidden, master_key,
                               key.encrypted_key.ciphertext, key.encrypted_key.nonce,
                               key.encrypted_key.tag, &retval);
        break;

    case AES_GCM_WITH_SW_ENFORCED:
        retval = AesGcmDecryptKey(key, hidden, master_key, error);
        break;
    }

    return retval;
}

}  // namespace keymaster
