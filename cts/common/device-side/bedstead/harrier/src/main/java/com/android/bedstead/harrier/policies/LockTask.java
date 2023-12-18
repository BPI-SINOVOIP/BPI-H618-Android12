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

package com.android.bedstead.harrier.policies;

import static com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy.DeviceOwnerControl.USER;
import static com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy.ProfileOwnerControl.AFFILIATED_OR_NO_DO;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;

import com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy;

/**
 * Policies around Lock Task mode
 * (https://developer.android.com/work/dpc/dedicated-devices/lock-task-mode).
 *
 * <p>This is used by methods such as
 * {@link DevicePolicyManager#setLockTaskFeatures(ComponentName, int)} and
 * {@link DevicePolicyManager#setLockTaskPackages(ComponentName, String[])}.
 */
@EnterprisePolicy(deviceOwner = USER, profileOwner = AFFILIATED_OR_NO_DO)
public final class LockTask {
}
