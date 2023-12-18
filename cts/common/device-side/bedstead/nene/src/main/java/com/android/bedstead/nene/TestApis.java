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

package com.android.bedstead.nene;

import com.android.bedstead.nene.activities.Activities;
import com.android.bedstead.nene.annotations.Experimental;
import com.android.bedstead.nene.context.Context;
import com.android.bedstead.nene.devicepolicy.DevicePolicy;
import com.android.bedstead.nene.packages.Packages;
import com.android.bedstead.nene.permissions.Permissions;
import com.android.bedstead.nene.users.Users;

/**
 * Entry point to Nene Test APIs.
 */
public final class TestApis {
    private final Context mContext = new Context(this);
    private final Users mUsers = new Users(this);
    private final Packages mPackages = new Packages(this);
    private final DevicePolicy mDevicePolicy = new DevicePolicy(this);
    private final Activities mActivities = new Activities(this);

    /** Access Test APIs related to Users. */
    public Users users() {
        return mUsers;
    }

    /** Access Test APIs related to Packages. */
    public Packages packages() {
        return mPackages;
    }

    /** Access Test APIs related to device policy. */
    public DevicePolicy devicePolicy() {
        return mDevicePolicy;
    }

    /** Access Test APIs related to permissions. */
    public Permissions permissions() {
        return Permissions.sInstance;
    }

    /** Access Test APIs related to context. */
    public Context context() {
        return mContext;
    }

    /** Access Test APIs related to activities. */
    // TODO(scottjonathan): Consider if Activities requires a top-level nene API or if it can go
    //  inside packages
    @Experimental
    public Activities activities() {
        return mActivities;
    }
}
