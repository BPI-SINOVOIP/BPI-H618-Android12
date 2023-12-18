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
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

public final class EnumQueryHelper <E extends Queryable, F> implements EnumQuery<E, F>, Serializable {

    private final E mQuery;
    private Set<F> mIsEqualTo = null;
    private Set<F> mIsNotEqualTo = null;

    public EnumQueryHelper(E query) {
        mQuery = query;
    }

    @Override
    public E isEqualTo(F value) {
        if (mIsEqualTo != null) {
            throw new IllegalStateException("isEqualTo or isOneOf already specified");
        }
        mIsEqualTo = new HashSet<>();
        mIsEqualTo.add(value);

        return mQuery;
    }

    @Override
    public E isNotEqualTo(F value) {
        if (mIsNotEqualTo != null) {
            throw new IllegalStateException("isNotEqualTo or isNotOneOf already specified");
        }
        mIsNotEqualTo = new HashSet<>();
        mIsNotEqualTo.add(value);

        return mQuery;
    }

    @Override
    public E isOneOf(F... values) {
        if (mIsEqualTo != null) {
            throw new IllegalStateException("isEqualTo or isOneOf already specified");
        }
        mIsEqualTo = new HashSet<>();
        mIsEqualTo.addAll(Arrays.asList(values));

        return mQuery;
    }

    @Override
    public E isNotOneOf(F... values) {
        if (mIsNotEqualTo != null) {
            throw new IllegalStateException("isNotEqualTo or isNotOneOf already specified");
        }
        mIsNotEqualTo = new HashSet<>();
        mIsNotEqualTo.addAll(Arrays.asList(values));

        return mQuery;
    }

    @Override
    public boolean matches(F value) {
        if (!checkIsEqualTo(value)) {
            return false;
        }

        if (!checkIsNotEqualTo(value)) {
            return false;
        }

        return true;
    }

    private boolean checkIsEqualTo(F value) {
        if (mIsEqualTo == null) {
            return true;
        }

        for (F v : mIsEqualTo) {
            if (v == value) {
                return true;
            }
        }

        return false;
    }

    private boolean checkIsNotEqualTo(F value) {
        if (mIsNotEqualTo == null) {
            return true;
        }

        for (F v : mIsNotEqualTo) {
            if (v == value) {
                return false;
            }
        }

        return true;
    }
}
