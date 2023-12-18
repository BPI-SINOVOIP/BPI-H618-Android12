/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning.manageduser;

import static java.util.Objects.requireNonNull;

import android.content.Context;
import android.os.UserHandle;

import com.android.internal.annotations.VisibleForTesting;
import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.task.nonrequiredapps.SystemAppsSnapshot;

/**
 * After a managed user is created, take a system app snapshot.
 */
// TODO(b/178711424): move this into the framework.
public class ManagedUserCreationController {

    final private SystemAppsSnapshot mSystemAppsSnapshot;
    final private int mUserId;
    final private boolean mLeaveAllSystemAppsEnabled;

    public ManagedUserCreationController(int userId, boolean leaveAllSystemAppsEnabled,
            Context context) {
        this(userId, leaveAllSystemAppsEnabled, new SystemAppsSnapshot(context));
    }

    @VisibleForTesting
    ManagedUserCreationController(int userId, boolean leaveAllSystemAppsEnabled,
            SystemAppsSnapshot systemAppsSnapshot) {
        mUserId = userId;
        mLeaveAllSystemAppsEnabled = leaveAllSystemAppsEnabled;
        mSystemAppsSnapshot = requireNonNull(systemAppsSnapshot);
    }

    public void run() {
        if (mUserId == UserHandle.USER_NULL) {
            ProvisionLogger.loge("Missing userId.");
            return;
        }
        if (!mLeaveAllSystemAppsEnabled) {
            mSystemAppsSnapshot.takeNewSnapshot(mUserId);
        }
    }
}
