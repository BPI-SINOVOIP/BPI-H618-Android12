/*
 * Copyright (C) 2021 The Android Open Source Project
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
package com.android.car.bluetooth;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import javax.crypto.spec.SecretKeySpec;

class FastPairUtils {
    static final String TAG = FastPairUtils.class.getSimpleName();
    static final boolean DBG = Log.isLoggable("FastPair", Log.DEBUG);
    static final String PREFERENCES = "com.android.car.bluetooth";
    static final String ACCOUNT_KEYS = "AccountKeysCount";
    static final String THREAD_NAME = "FastPairProvider";

    private static final byte SALT_FIELD_DESCRIPTOR = 0x11;
    private static final int BD_ADDR_LEN = 6;
    private static final int BD_UUID_LEN = 16;

    //construct the advertisement based on stored account keys
    static byte[] getAccountKeyAdvertisement(Context context) {
        byte[] salt = new byte[1];
        List<FastPairUtils.AccountKey> keys = new ArrayList<>();
        new Random().nextBytes(salt);
        keys = readStoredAccountKeys(context);

        //calculate bloom results
        byte[] bloomResults = bloom(keys, salt[0]);
        int size = bloomResults.length;

        //assemble advertisement
        ByteBuffer accountKeyAdvertisement = ByteBuffer.allocate(size + 4);
        accountKeyAdvertisement.put((byte) 0); //reserved Flags byte
        accountKeyAdvertisement.put((byte) (size << 4));
        accountKeyAdvertisement.put(bloomResults);
        accountKeyAdvertisement.put(SALT_FIELD_DESCRIPTOR);
        accountKeyAdvertisement.put(salt);

        return accountKeyAdvertisement.array();
    }

    //given a list of account keys and a salt, calculate the bloom results
    static byte[] bloom(List<AccountKey> keys, byte salt) {
        int size = (int) 1.2 * keys.size() + 3;
        byte[] filter = new byte[size];

        for (AccountKey key : keys) {
            byte[] v = Arrays.copyOf(key.key, 17);
            v[16] = salt;
            try {
                byte[] hashed = MessageDigest.getInstance("SHA-256").digest(v);
                ByteBuffer byteBuffer = ByteBuffer.wrap(hashed);
                for (int j = 0; j < 8; j++) {
                    long k = Integer.toUnsignedLong(byteBuffer.getInt()) % (size * 8);
                    filter[(int) (k / 8)] |= 1 << (k % 8);
                }
            } catch (Exception e) {
                Log.e(TAG, e.toString());
            }
        }
        return filter;
    }

    static List<AccountKey> readStoredAccountKeys(Context context) {
        List<AccountKey> keys = new ArrayList<>();
        SharedPreferences sharedPref = context.getSharedPreferences(PREFERENCES,
                Context.MODE_PRIVATE);
        int accountKeyCount = sharedPref.getInt(ACCOUNT_KEYS, 0);

        for (int i = 1; i <= accountKeyCount; i++) {
            String readAccountKey = sharedPref.getString("" + i, null);
            if (readAccountKey != null) {
                keys.add(new FastPairUtils.AccountKey(readAccountKey));
            } else {
                Log.w(TAG, "Read account key == " + readAccountKey);
            }
        }

        Log.d(TAG, "Read " + keys.size() + "/" + accountKeyCount + " keys.");
        return keys;
    }

    static void writeStoredAccountKeys(Context context, List<AccountKey> keys) {
        SharedPreferences sharedPref = context
                .getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        int accountKeyCount = keys.size();
        SharedPreferences.Editor editor = sharedPref.edit();
        for (int i = 0; i < accountKeyCount; i++) {
            editor.putString("" + accountKeyCount,
                    new BigInteger(keys.get(i).toBytes()).toString());
        }
        editor.putInt(ACCOUNT_KEYS, accountKeyCount);
        editor.apply();
    }

    static byte[] getBytesFromAddress(String address) {
        int i, j = 0;
        byte[] output = new byte[BD_ADDR_LEN];

        for (i = 0; i < address.length(); i++) {
            if (address.charAt(i) != ':') {
                output[j] = (byte) Integer.parseInt(address.substring(i, i + 2), BD_UUID_LEN);
                j++;
                i++;
            }
        }
        return output;
    }

    static class AccountKey {

        public final byte[] key;

        AccountKey(byte[] newKey) {
            key = newKey;
        }

        AccountKey(String newKey) {
            key = new BigInteger(newKey).toByteArray();
        }

        public byte[] toBytes() {
            return key;
        }

        public SecretKeySpec getKeySpec() {
            return new SecretKeySpec(key, "AES");
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof AccountKey)) {
                return false;
            }
            return Arrays.equals(key, ((AccountKey) obj).key);
        }
    }
}
