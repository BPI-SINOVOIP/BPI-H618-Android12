/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <algorithm>

#include <gtest/gtest.h>

#include <openssl/engine.h>
#include <openssl/rand.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>
#include <keymaster/key_blob_utils/auth_encrypted_key_blob.h>
#include <keymaster/key_blob_utils/integrity_assured_key_blob.h>
#include <keymaster/keymaster_tags.h>
#include <keymaster/km_openssl/software_random_source.h>

#include "android_keymaster_test_utils.h"

namespace keymaster {

namespace test {

namespace {

const uint8_t master_key_data[16] = {};
const uint8_t key_data[5] = {21, 22, 23, 24, 25};

}  // namespace

class KeyBlobTest : public testing::Test, public SoftwareRandomSource {
  protected:
    KeyBlobTest()
        : key_material_(key_data, array_length(key_data)),
          master_key_(master_key_data, array_length(master_key_data)) {
        hw_enforced_.push_back(TAG_ALGORITHM, KM_ALGORITHM_RSA);
        hw_enforced_.push_back(TAG_KEY_SIZE, 256);
        hw_enforced_.push_back(TAG_BLOB_USAGE_REQUIREMENTS, KM_BLOB_STANDALONE);
        hw_enforced_.push_back(TAG_MIN_SECONDS_BETWEEN_OPS, 10);
        hw_enforced_.push_back(TAG_ALL_USERS);
        hw_enforced_.push_back(TAG_NO_AUTH_REQUIRED);
        hw_enforced_.push_back(TAG_ORIGIN, KM_ORIGIN_GENERATED);

        sw_enforced_.push_back(TAG_ACTIVE_DATETIME, 10);
        sw_enforced_.push_back(TAG_ORIGINATION_EXPIRE_DATETIME, 100);
        sw_enforced_.push_back(TAG_CREATION_DATETIME, 10);

        hidden_.push_back(TAG_ROOT_OF_TRUST, "foo", 3);
        hidden_.push_back(TAG_APPLICATION_ID, "my_app", 6);
    }

    keymaster_error_t Encrypt(AuthEncryptedBlobFormat format = AES_GCM_WITH_SW_ENFORCED) {
        keymaster_error_t error;
        encrypted_key_ = EncryptKey(key_material_, format, hw_enforced_, sw_enforced_, hidden_,
                                    master_key_, *this, &error);
        return error;
    }

    keymaster_error_t Decrypt() {
        keymaster_error_t error;
        decrypted_plaintext_ = DecryptKey(move(deserialized_key_), hidden_, master_key_, &error);
        return error;
    }

    keymaster_error_t Serialize() {
        keymaster_error_t error;
        serialized_blob_ =
            SerializeAuthEncryptedBlob(encrypted_key_, hw_enforced_, sw_enforced_, &error);
        return error;
    }

    keymaster_error_t Deserialize() {
        keymaster_error_t error;
        deserialized_key_ = DeserializeAuthEncryptedBlob(serialized_blob_, &error);
        return error;
    }

    // Encryption inputs
    AuthorizationSet hw_enforced_;
    AuthorizationSet sw_enforced_;
    AuthorizationSet hidden_;
    KeymasterKeyBlob key_material_;
    KeymasterKeyBlob master_key_;

    // Encryption output
    EncryptedKey encrypted_key_;

    // Serialization output
    KeymasterKeyBlob serialized_blob_;

    // Deserialization output
    DeserializedKey deserialized_key_;

    // Decryption output.
    KeymasterKeyBlob decrypted_plaintext_;
};

TEST_F(KeyBlobTest, EncryptDecrypt) {
    for (auto format : {AES_OCB, AES_GCM_WITH_SW_ENFORCED}) {
        ASSERT_EQ(KM_ERROR_OK, Encrypt(format));
        ASSERT_EQ(KM_ERROR_OK, Serialize());

        // key_data shouldn't be anywhere in the blob, ciphertext should.
        EXPECT_EQ(serialized_blob_.end(),
                  std::search(serialized_blob_.begin(), serialized_blob_.end(),
                              key_material_.begin(), key_material_.end()));
        EXPECT_NE(serialized_blob_.end(),
                  std::search(serialized_blob_.begin(), serialized_blob_.end(),
                              encrypted_key_.ciphertext.begin(), encrypted_key_.ciphertext.end()));

        keymaster_error_t error;
        DeserializedKey deserialized = DeserializeAuthEncryptedBlob(serialized_blob_, &error);
        ASSERT_EQ(KM_ERROR_OK, error);
        EXPECT_EQ(hw_enforced_, deserialized.hw_enforced);
        switch (format) {
        case AES_OCB:
        case AES_GCM_WITH_SW_ENFORCED:
            EXPECT_EQ(sw_enforced_, deserialized.sw_enforced);
            break;
        }

        KeymasterKeyBlob plaintext = DecryptKey(move(deserialized), hidden_, master_key_, &error);

        ASSERT_EQ(key_material_.size(), plaintext.size());
        EXPECT_TRUE(std::equal(key_material_.begin(), key_material_.end(), plaintext.begin()));
    }
}

TEST_F(KeyBlobTest, WrongKeyLength) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    // Modify the key length, shouldn't be able to parse.
    serialized_blob_.writable_data()[1 /* version */ + 4 /* nonce len */ + 12 /* nonce */ + 3]++;

    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, Deserialize());
}

TEST_F(KeyBlobTest, WrongNonce) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    // Find the nonce, then modify it.
    auto nonce_ptr = std::search(serialized_blob_.begin(), serialized_blob_.end(),
                                 encrypted_key_.nonce.begin(), encrypted_key_.nonce.end());
    ASSERT_NE(nonce_ptr, serialized_blob_.end());
    (*const_cast<uint8_t*>(nonce_ptr))++;

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, Decrypt());
}

TEST_F(KeyBlobTest, WrongTag) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    // Find the tag, then modify it.
    auto tag_ptr = std::search(serialized_blob_.begin(), serialized_blob_.end(),
                               encrypted_key_.tag.begin(), encrypted_key_.tag.end());
    ASSERT_NE(tag_ptr, serialized_blob_.end());
    (*const_cast<uint8_t*>(tag_ptr))++;

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, Decrypt());
}

TEST_F(KeyBlobTest, WrongCiphertext) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    // Find the ciphertext, then modify it.
    auto ciphertext_ptr =
        std::search(serialized_blob_.begin(), serialized_blob_.end(),
                    encrypted_key_.ciphertext.begin(), encrypted_key_.ciphertext.end());
    ASSERT_NE(ciphertext_ptr, serialized_blob_.end());
    (*const_cast<uint8_t*>(ciphertext_ptr))++;

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, Decrypt());
}

TEST_F(KeyBlobTest, WrongMasterKey) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    uint8_t wrong_master_data[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    KeymasterKeyBlob wrong_master(wrong_master_data, array_length(wrong_master_data));

    // Decrypting with wrong master key should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    keymaster_error_t error;
    DecryptKey(move(deserialized_key_), hidden_, wrong_master, &error);

    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, error);
}

TEST_F(KeyBlobTest, WrongHwEnforced) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    // Find enforced serialization data and modify it.
    size_t hw_enforced_size = hw_enforced_.SerializedSize();
    UniquePtr<uint8_t[]> hw_enforced_data(new uint8_t[hw_enforced_size]);
    hw_enforced_.Serialize(hw_enforced_data.get(), hw_enforced_data.get() + hw_enforced_size);

    auto hw_enforced_ptr =
        std::search(serialized_blob_.begin(), serialized_blob_.end(), hw_enforced_data.get(),
                    hw_enforced_data.get() + hw_enforced_size);
    ASSERT_NE(serialized_blob_.end(), hw_enforced_ptr);
    (*(const_cast<uint8_t*>(hw_enforced_ptr) + hw_enforced_size - 1))++;

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, Decrypt());
}

TEST_F(KeyBlobTest, WrongSwEnforced) {
    for (auto format : {AES_OCB, AES_GCM_WITH_SW_ENFORCED}) {
        ASSERT_EQ(KM_ERROR_OK, Encrypt(format));
        ASSERT_EQ(KM_ERROR_OK, Serialize());

        // Find enforced serialization data and modify it.
        size_t sw_enforced_size = sw_enforced_.SerializedSize();
        UniquePtr<uint8_t[]> sw_enforced_data(new uint8_t[sw_enforced_size]);
        sw_enforced_.Serialize(sw_enforced_data.get(), sw_enforced_data.get() + sw_enforced_size);

        auto sw_enforced_ptr =
            std::search(serialized_blob_.begin(), serialized_blob_.end(), sw_enforced_data.get(),
                        sw_enforced_data.get() + sw_enforced_size);
        ASSERT_NE(serialized_blob_.end(), sw_enforced_ptr);
        (*(const_cast<uint8_t*>(sw_enforced_ptr) + sw_enforced_size - 1))++;

        // Deserialization shouldn't be affected, but decryption should fail.
        ASSERT_EQ(KM_ERROR_OK, Deserialize());
        ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, Decrypt());
    }
}

TEST_F(KeyBlobTest, EmptyHidden) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    AuthorizationSet wrong_hidden;

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    keymaster_error_t error;
    DecryptKey(move(deserialized_key_), wrong_hidden, master_key_, &error);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, error);
}

TEST_F(KeyBlobTest, WrongRootOfTrust) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    AuthorizationSet wrong_hidden;
    wrong_hidden.push_back(TAG_ROOT_OF_TRUST, "bar", 2);
    wrong_hidden.push_back(TAG_APPLICATION_ID, "my_app", 6);

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    keymaster_error_t error;
    DecryptKey(move(deserialized_key_), wrong_hidden, master_key_, &error);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, error);
}

TEST_F(KeyBlobTest, WrongAppId) {
    ASSERT_EQ(KM_ERROR_OK, Encrypt());
    ASSERT_EQ(KM_ERROR_OK, Serialize());

    AuthorizationSet wrong_hidden;
    wrong_hidden.push_back(TAG_ROOT_OF_TRUST, "foo", 3);
    wrong_hidden.push_back(TAG_APPLICATION_ID, "your_app", 7);

    // Deserialization shouldn't be affected, but decryption should fail.
    ASSERT_EQ(KM_ERROR_OK, Deserialize());
    keymaster_error_t error;
    DecryptKey(move(deserialized_key_), wrong_hidden, master_key_, &error);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, error);
}

// This test is especially useful when compiled for 32-bit mode and run under valgrind.
TEST_F(KeyBlobTest, FuzzTest) {
    time_t now = time(NULL);
    std::cout << "Seeding rand() with " << now << " for fuzz test." << std::endl;
    srand(now);

    // Fill large buffer with random bytes.
    const int kBufSize = 10000;
    UniquePtr<uint8_t[]> buf(new uint8_t[kBufSize]);
    for (size_t i = 0; i < kBufSize; ++i)
        buf[i] = static_cast<uint8_t>(rand());

    // Try to deserialize every offset with multiple methods.
    size_t deserialize_auth_encrypted_success = 0;
    for (size_t i = 0; i < kBufSize; ++i) {
        keymaster_key_blob_t blob = {buf.get() + i, kBufSize - i};
        KeymasterKeyBlob key_blob(blob);

        // Integrity-assured blob.
        ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB,
                  DeserializeIntegrityAssuredBlob(key_blob, hidden_, &key_material_, &hw_enforced_,
                                                  &sw_enforced_));

        // Auth-encrypted blob.
        keymaster_error_t error;
        auto deserialized = DeserializeAuthEncryptedBlob(key_blob, &error);
        if (error == KM_ERROR_OK) {
            // It's possible to deserialize successfully.  Decryption should always fail.
            ++deserialize_auth_encrypted_success;
            DecryptKey(move(deserialized), hidden_, master_key_, &error);
        }
        ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB, error)
            << "Somehow sucessfully parsed a blob with seed " << now << " at offset " << i;
    }
}

TEST_F(KeyBlobTest, UnderflowTest) {
    uint8_t buf[0];
    keymaster_key_blob_t blob = {buf, 0};
    KeymasterKeyBlob key_blob(blob);
    EXPECT_NE(nullptr, key_blob.key_material);
    EXPECT_EQ(0U, key_blob.key_material_size);

    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB,
              DeserializeIntegrityAssuredBlob(key_blob, hidden_, &key_material_, &hw_enforced_,
                                              &sw_enforced_));

    keymaster_error_t error;
    DeserializeAuthEncryptedBlob(key_blob, &error);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, error);
}

TEST_F(KeyBlobTest, DupBufferToolarge) {
    uint8_t buf[0];
    keymaster_key_blob_t blob = {buf, 0};
    blob.key_material_size = 16 * 1024 * 1024 + 1;
    KeymasterKeyBlob key_blob(blob);
    EXPECT_EQ(nullptr, key_blob.key_material);
    EXPECT_EQ(0U, key_blob.key_material_size);

    ASSERT_EQ(KM_ERROR_INVALID_KEY_BLOB,
              DeserializeIntegrityAssuredBlob(key_blob, hidden_, &key_material_, &hw_enforced_,
                                              &sw_enforced_));

    keymaster_error_t error;
    DeserializeAuthEncryptedBlob(key_blob, &error);
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB, error);
}

}  // namespace test
}  // namespace keymaster
