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

import com.android.queryable.util.SerializableParcelWrapper;
import com.android.queryable.Queryable;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;

/** Implementation of {@link BundleQuery}. */
public final class BundleQueryHelper<E extends Queryable> implements BundleQuery<E>,
        Serializable {

    private final E mQuery;
    private final Map<String, BundleKeyQueryHelper<E>> mKeyQueryHelpers = new HashMap<>();

    BundleQueryHelper() {
        mQuery = (E) this;
    }

    public BundleQueryHelper(E query) {
        mQuery = query;
    }

    @Override
    public BundleKeyQuery<E> key(String key) {
        if (!mKeyQueryHelpers.containsKey(key)) {
            mKeyQueryHelpers.put(key, new BundleKeyQueryHelper<>(mQuery));
        }
        return mKeyQueryHelpers.get(key);
    }

    @Override
    public boolean matches(Bundle value) {
        for (Map.Entry<String, BundleKeyQueryHelper<E>> keyQueries : mKeyQueryHelpers.entrySet()) {
            if (!keyQueries.getValue().matches(value, keyQueries.getKey())) {
                return false;
            }
        }

        return true;
    }

    public boolean matches(SerializableParcelWrapper<Bundle> serializableBundle) {
        if ((serializableBundle == null || serializableBundle.get() == null)) {
            if (mKeyQueryHelpers.isEmpty()) {
                return true;
            }
            return false;
        }

        return matches(serializableBundle.get());
    }
}
