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

import android.os.UserHandle;

import com.android.queryable.util.SerializableParcelWrapper;
import com.android.queryable.Queryable;

import java.io.Serializable;

/** Implementation of {@link UserHandleQuery}. */
public final class UserHandleQueryHelper<E extends Queryable>
        implements UserHandleQuery<E>, Serializable {

    private final E mQuery;
    private UserHandle mEqualsValue = null;
    private IntegerQueryHelper<E> mIdQuery = null;

    UserHandleQueryHelper() {
        mQuery = (E) this;
    }

    public UserHandleQueryHelper(E query) {
        mQuery = query;
    }

    @Override
    public E isEqualTo(UserHandle userHandle) {
        mEqualsValue = userHandle;
        return mQuery;
    }

    @Override
    public IntegerQuery<E> id() {
        if (mIdQuery == null) {
            mIdQuery = new IntegerQueryHelper<>(mQuery);
        }
        return mIdQuery;
    }

    /**
     * {@code true} if all filters are met by the {@link UserHandle}.
     */
    @Override
    public boolean matches(UserHandle value) {
        if (mEqualsValue != null && !mEqualsValue.equals(value)) {
            return false;
        }

        if (mIdQuery != null && !mIdQuery.matches(value.getIdentifier())) {
            return false;
        }

        return true;
    }

    /**
     * {@code true} if all filters are met by the {@link UserHandle} contained in
     * {@code serializableBundle}.
     */
    public boolean matches(SerializableParcelWrapper<UserHandle> serializableBundle) {
        if ((serializableBundle == null || serializableBundle.get() == null)) {
            return mEqualsValue == null && mIdQuery == null;
        }

        return matches(serializableBundle.get());
    }
}
