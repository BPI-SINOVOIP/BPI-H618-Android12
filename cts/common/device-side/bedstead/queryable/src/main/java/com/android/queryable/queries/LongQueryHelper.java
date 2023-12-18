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

import androidx.annotation.Nullable;

import com.android.queryable.Queryable;

import java.io.Serializable;

/** Implementation of {@link LongQuery}. */
public final class LongQueryHelper<E extends Queryable> implements LongQuery<E>, Serializable {

    @Nullable private Long mEqualToValue = null;
    @Nullable private Long mGreaterThanValue = null;
    @Nullable private Long mGreaterThanOrEqualToValue = null;
    @Nullable private Long mLessThanValue = null;
    @Nullable private Long mLessThanOrEqualToValue = null;

    private final E mQuery;

    public LongQueryHelper(E query) {
        mQuery = query;
    }


    @Override
    public E isEqualTo(long i) {
        mEqualToValue = i;
        return mQuery;
    }

    @Override
    public E isGreaterThan(long i) {
        if (mGreaterThanValue == null) {
            mGreaterThanValue = i;
        } else {
            mGreaterThanValue = Math.max(mGreaterThanValue, i);
        }
        return mQuery;
    }

    @Override
    public E isGreaterThanOrEqualTo(long i) {
        if (mGreaterThanOrEqualToValue == null) {
            mGreaterThanOrEqualToValue = i;
        } else {
            mGreaterThanOrEqualToValue = Math.max(mGreaterThanOrEqualToValue, i);
        }
        return mQuery;
    }

    @Override
    public E isLessThan(long i) {
        if (mLessThanValue == null) {
            mLessThanValue = i;
        } else {
            mLessThanValue = Math.min(mLessThanValue, i);
        }
        return mQuery;
    }

    @Override
    public E isLessThanOrEqualTo(long i) {
        if (mLessThanOrEqualToValue == null) {
            mLessThanOrEqualToValue = i;
        } else {
            mLessThanOrEqualToValue = Math.min(mLessThanOrEqualToValue, i);
        }
        return mQuery;
    }

    /** {@code true} if all filters are met by {@code value}. */
    public boolean matches(long value) {
        if (mEqualToValue != null && mEqualToValue != value) {
            return false;
        }

        if (mGreaterThanValue != null && value <= mGreaterThanValue) {
            return false;
        }

        if (mGreaterThanOrEqualToValue != null && value < mGreaterThanOrEqualToValue) {
            return false;
        }

        if (mLessThanValue != null && value >= mLessThanValue) {
            return false;
        }

        if (mLessThanOrEqualToValue != null && value > mLessThanOrEqualToValue) {
            return false;
        }

        return true;
    }
}
