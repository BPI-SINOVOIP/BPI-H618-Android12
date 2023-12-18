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

import android.content.Intent;

import com.android.queryable.util.SerializableParcelWrapper;
import com.android.queryable.Queryable;

import java.io.Serializable;

/** Implementation of {@link IntentQuery}. */
public final class IntentQueryHelper<E extends Queryable> implements IntentQuery<E>,
        Serializable {

    private final E mQuery;
    private final StringQueryHelper<E> mAction;
    private final BundleQueryHelper<E> mExtras;

    IntentQueryHelper() {
        mQuery = (E) this;
        mAction = new StringQueryHelper<>(mQuery);
        mExtras = new BundleQueryHelper<>(mQuery);
    }

    public IntentQueryHelper(E query) {
        mQuery = query;
        mAction = new StringQueryHelper<>(query);
        mExtras = new BundleQueryHelper<>(query);
    }

    @Override
    public StringQuery<E> action() {
        return mAction;
    }

    @Override
    public BundleQuery<E> extras() {
        return mExtras;
    }

    /** {@code true} if all filters are met by {@code value}. */
    @Override
    public boolean matches(Intent value) {
        if (!mAction.matches(value.getAction())) {
            return false;
        }
        if (!mExtras.matches(value.getExtras())) {
            return false;
        }

        return true;
    }

    /**
     * {@code true} if all filters are met by the {@link Intent} contained in
     * {@code serializableBundle}.
     */
    public boolean matches(SerializableParcelWrapper<Intent> serializableBundle) {
        if ((serializableBundle == null || serializableBundle.get() == null)) {
            return false;
        }

        return matches(serializableBundle.get());
    }
}
