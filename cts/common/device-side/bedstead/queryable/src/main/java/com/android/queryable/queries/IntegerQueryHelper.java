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

import com.android.queryable.Queryable;

import java.io.Serializable;

/** Implementation of {@link IntegerQuery}. */
public final class IntegerQueryHelper<E extends Queryable> implements IntegerQuery<E>,
        Serializable {

    private Integer mEqualToValue = null;
    private Integer mGreaterThanValue = null;
    private Integer mGreaterThanOrEqualToValue = null;
    private Integer mLessThanValue = null;
    private Integer mLessThanOrEqualToValue = null;

    private final E mQuery;

    IntegerQueryHelper() {
        mQuery = (E) this;
    }

    public IntegerQueryHelper(E query) {
        mQuery = query;
    }


    @Override
    public E isEqualTo(int i) {
        mEqualToValue = i;
        return mQuery;
    }

    @Override
    public E isGreaterThan(int i) {
        if (mGreaterThanValue == null) {
            mGreaterThanValue = i;
        } else {
            mGreaterThanValue = Math.max(mGreaterThanValue, i);
        }
        return mQuery;
    }

    @Override
    public E isGreaterThanOrEqualTo(int i) {
        if (mGreaterThanOrEqualToValue == null) {
            mGreaterThanOrEqualToValue = i;
        } else {
            mGreaterThanOrEqualToValue = Math.max(mGreaterThanOrEqualToValue, i);
        }
        return mQuery;
    }

    @Override
    public E isLessThan(int i) {
        if (mLessThanValue == null) {
            mLessThanValue = i;
        } else {
            mLessThanValue = Math.min(mLessThanValue, i);
        }
        return mQuery;
    }

    @Override
    public E isLessThanOrEqualTo(int i) {
        if (mLessThanOrEqualToValue == null) {
            mLessThanOrEqualToValue = i;
        } else {
            mLessThanOrEqualToValue = Math.min(mLessThanOrEqualToValue, i);
        }
        return mQuery;
    }

    /** {@code true} if all filters are met by {@code value}. */
    @Override
    public boolean matches(Integer value) {
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
