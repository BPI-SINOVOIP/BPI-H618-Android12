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

public final class SetQueryHelper<E extends Queryable, F, G extends Query<F>> implements SetQuery<E, F, G>, Serializable {

    private E mQuery;
    private final IntegerQueryHelper<E> mSizeQuery;
    private final Set<G> mContains = new HashSet<>();
    private final Set<G> mDoesNotContain = new HashSet<>();

    SetQueryHelper() {
        mQuery = (E) this;
        mSizeQuery = new IntegerQueryHelper<>(mQuery);
    }

    public SetQueryHelper(E query) {
        mQuery = query;
        mSizeQuery = new IntegerQueryHelper<>(mQuery);
    }

    @Override
    public IntegerQuery<E> size() {
        return mSizeQuery;
    }

    @Override
    public E contains(G... objects) {
        mContains.addAll(Arrays.asList(objects));
        return mQuery;
    }

    @Override
    public E doesNotContain(G... objects) {
        mDoesNotContain.addAll(Arrays.asList(objects));
        return mQuery;
    }

    @Override
    public boolean matches(Set<F> value) {
        if (!mSizeQuery.matches(value.size())) {
            return false;
        }

        if (!checkContainsAtLeast(value)) {
            return false;
        }

        if (!checkDoesNotContain(value)) {
            return false;
        }

        return true;
    }

    private boolean checkContainsAtLeast(Set<F> value) {
        Set<F> v = new HashSet<>(value);

        for (G containsAtLeast : mContains) {
            F match = findMatch(containsAtLeast, v);

            if (match == null) {
                return false;
            }
            v.remove(match);
        }

        return true;
    }

    private boolean checkDoesNotContain(Set<F> value) {
        for (G doesNotContain : mDoesNotContain) {
            if (findMatch(doesNotContain, value) != null) {
                return false;
            }
        }

        return true;
    }

    private F findMatch(G query, Set<F> values) {
        for (F value : values) {
            if (query.matches(value)) {
                return value;
            }
        }

        return null;
    }
}
