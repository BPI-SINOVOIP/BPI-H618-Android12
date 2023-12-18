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
import com.android.queryable.info.ClassInfo;

import java.io.Serializable;

/** Implementation of {@link ClassQuery}. */
public final class ClassQueryHelper<E extends Queryable>
        implements ClassQuery<E>, Serializable {

    private final E mQuery;
    private final StringQueryHelper<E> mClassName;
    private final StringQueryHelper<E> mSimpleName;

    ClassQueryHelper() {
        mQuery = (E) this;
        mClassName = new StringQueryHelper<>(mQuery);
        mSimpleName = new StringQueryHelper<>(mQuery);
    }

    public ClassQueryHelper(E query) {
        mQuery = query;
        mClassName = new StringQueryHelper<>(query);
        mSimpleName = new StringQueryHelper<>(query);
    }

    @Override
    public E isSameClassAs(Class<?> clazz) {
        return className().isEqualTo(clazz.getName());
    }

    @Override
    public StringQuery<E> className() {
        return mClassName;
    }

    @Override
    public StringQuery<E> simpleName() {
        return mSimpleName;
    }

    @Override
    public boolean matches(ClassInfo value) {
        if (!mClassName.matches(value.className())) {
            return false;
        }

        if (!mSimpleName.matches(value.simpleName())) {
            return false;
        }

        return true;
    }
}
