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
import com.android.queryable.info.DeviceAdminReceiverInfo;

/** Implementation of {@link DeviceAdminReceiverQuery}. */
public final class DeviceAdminReceiverQueryHelper<E extends Queryable>
        implements DeviceAdminReceiverQuery<E> {

    private final E mQuery;
    private final BroadcastReceiverQueryHelper<E> mBroadcastReceiverQueryHelper;

    DeviceAdminReceiverQueryHelper() {
        mQuery = (E) this;
        mBroadcastReceiverQueryHelper = new BroadcastReceiverQueryHelper<>(mQuery);
    }

    public DeviceAdminReceiverQueryHelper(E query) {
        mQuery = query;
        mBroadcastReceiverQueryHelper = new BroadcastReceiverQueryHelper<>(query);
    }

    @Override
    public BroadcastReceiverQuery<E> broadcastReceiver() {
        return mBroadcastReceiverQueryHelper;
    }

    /** {@code true} if all filters are met by {@code value}. */
    @Override
    public boolean matches(DeviceAdminReceiverInfo value) {
        return mBroadcastReceiverQueryHelper.matches(value);
    }
}
