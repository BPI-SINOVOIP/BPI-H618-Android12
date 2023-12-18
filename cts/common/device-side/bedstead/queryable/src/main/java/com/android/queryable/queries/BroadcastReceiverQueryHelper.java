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
import com.android.queryable.info.BroadcastReceiverInfo;

/** Implementation of {@link BroadcastReceiverQuery}. */
public final class BroadcastReceiverQueryHelper<E extends Queryable>
        implements BroadcastReceiverQuery<E> {

    private final E mQuery;
    private final ClassQueryHelper<E> mReceiverClassQueryHelper;

    BroadcastReceiverQueryHelper() {
        mQuery = (E) this;
        mReceiverClassQueryHelper = new ClassQueryHelper<>(mQuery);
    }

    public BroadcastReceiverQueryHelper(E query) {
        mQuery = query;
        mReceiverClassQueryHelper = new ClassQueryHelper<>(query);
    }

    @Override
    public ClassQuery<E> receiverClass() {
        return mReceiverClassQueryHelper;
    }

    /** {@code true} if all filters are met by {@code value}. */
    @Override
    public boolean matches(BroadcastReceiverInfo value) {
        return mReceiverClassQueryHelper.matches(value);
    }
}
