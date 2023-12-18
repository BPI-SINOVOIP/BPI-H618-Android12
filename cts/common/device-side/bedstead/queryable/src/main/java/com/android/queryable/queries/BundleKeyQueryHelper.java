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

package com.android.queryable.queries;

import android.os.Bundle;

import com.android.queryable.Queryable;

import java.io.Serializable;

/** Implementation of {@link BundleKeyQuery}. */
public final class BundleKeyQueryHelper<E extends Queryable> implements BundleKeyQuery<E>,
        Serializable {

    private final E mQuery;
    private Boolean mExpectsToExist = null;
    private StringQueryHelper<E> mStringQuery = null;
    private SerializableQueryHelper<E> mSerializableQuery;
    private BundleQueryHelper<E> mBundleQuery;

    public BundleKeyQueryHelper(E query) {
        mQuery = query;
    }

    @Override
    public E exists() {
        if (mExpectsToExist != null) {
            throw new IllegalStateException(
                    "Cannot call exists() after calling exists() or doesNotExist()");
        }
        mExpectsToExist = true;
        return mQuery;
    }

    @Override
    public E doesNotExist() {
        if (mExpectsToExist != null) {
            throw new IllegalStateException(
                    "Cannot call doesNotExist() after calling exists() or doesNotExist()");
        }
        mExpectsToExist = false;
        return mQuery;
    }

    @Override
    public StringQuery<E> stringValue() {
        if (mStringQuery == null) {
            checkUntyped();
            mStringQuery = new StringQueryHelper<>(mQuery);
        }
        return mStringQuery;
    }

    @Override
    public SerializableQuery<E> serializableValue() {
        if (mSerializableQuery == null) {
            checkUntyped();
            mSerializableQuery = new SerializableQueryHelper<>(mQuery);
        }
        return mSerializableQuery;
    }

    @Override
    public BundleQuery<E> bundleValue() {
        if (mBundleQuery == null) {
            checkUntyped();
            mBundleQuery = new BundleQueryHelper<>(mQuery);
        }
        return mBundleQuery;
    }

    private void checkUntyped() {
        if (mStringQuery != null || mSerializableQuery != null || mBundleQuery != null) {
            throw new IllegalStateException("Each key can only be typed once");
        }
    }

    public boolean matches(Bundle value, String key) {
        if (mExpectsToExist != null && value.containsKey(key) != mExpectsToExist) {
            return false;
        }
        if (mStringQuery != null && !mStringQuery.matches(value.getString(key))) {
            return false;
        }
        if (mSerializableQuery != null && !mSerializableQuery.matches(value.getSerializable(key))) {
            return false;
        }
        if (mBundleQuery != null && !mBundleQuery.matches(value.getBundle(key))) {
            return false;
        }

        return true;
    }
}
