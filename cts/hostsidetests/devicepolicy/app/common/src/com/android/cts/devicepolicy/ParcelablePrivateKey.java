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

package com.android.cts.devicepolicy;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;

import javax.security.auth.DestroyFailedException;

/**
 * A {@link PrivateKey} that can be transmitted as a {@link Parcelable}.
 *
 * <p>TODO(b/191150645): it's needed because the {@code PrivateKey} returned by {@code KeyFactory}
 * does not properly implements {@code Serializable}.
 */
public final class ParcelablePrivateKey implements PrivateKey, Parcelable {

    private static final long serialVersionUID = 1L;

    private static final String TAG = ParcelablePrivateKey.class.getSimpleName();

    private final String mAlgorithm;
    private final byte[] mEncodedKey;
    private final PrivateKey mPrivateKey;

    /**
     * Default constructor.
     */
    public ParcelablePrivateKey(String algorithm, byte[] encodedKey)
            throws NoSuchAlgorithmException, InvalidKeySpecException {
        mAlgorithm = algorithm;
        mEncodedKey = encodedKey;
        mPrivateKey = KeyFactory.getInstance(algorithm).generatePrivate(
                new PKCS8EncodedKeySpec(encodedKey));
        Log.d(TAG, "ParcelablePrivateKey(): algorithm=" + algorithm
                + ", encodedKey.length=" + encodedKey.length
                + " pk.format=" + mPrivateKey.getFormat()
                + " pk.length=" + mPrivateKey.getEncoded().length);
    }

    @Override
    public String getAlgorithm() {
        return mPrivateKey.getAlgorithm();
    }

    @Override
    public String getFormat() {
        return mPrivateKey.getFormat();
    }

    @Override
    public byte[] getEncoded() {
        return mPrivateKey.getEncoded();
    }

    @Override
    public void destroy() throws DestroyFailedException {
        mPrivateKey.destroy();
    }

    @Override
    public boolean isDestroyed() {
        return mPrivateKey.isDestroyed();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mAlgorithm);
        dest.writeInt(mEncodedKey.length);
        dest.writeByteArray(mEncodedKey);
    }

    @Override
    public String toString() {
        return "ParcelablePrivateKey[algorithm=" + mAlgorithm
                + ", length=" + mEncodedKey.length + ']';
    }

    public static final Parcelable.Creator<ParcelablePrivateKey> CREATOR =
            new Parcelable.Creator<ParcelablePrivateKey>() {

        public ParcelablePrivateKey createFromParcel(Parcel in) {
            String algorithm = in.readString();
            int arrayLength = in.readInt();
            byte[] key = new byte[arrayLength];
            in.readByteArray(key);
            try {
                return new ParcelablePrivateKey(algorithm, key);
            } catch (Exception e) {
                throw new IllegalArgumentException("could not parse key: algorithm=" + algorithm
                        + ", key_size=" + arrayLength, e);
            }
        }

        public ParcelablePrivateKey[] newArray(int size) {
            return new ParcelablePrivateKey[size];
        }
    };

}
