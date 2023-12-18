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

import static com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy.DeviceOwnerControl.GLOBAL;
import static com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy.ProfileOwnerControl.NO;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;

import com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy;

import java.util.List;

/**
 * Policy for user control disabled packages.
 *
 * <p>This is used by methods such as
 * {@link DevicePolicyManager#setUserControlDisabledPackages(ComponentName, List)} and
 * {@link DevicePolicyManager#getUserControlDisabledPackages(ComponentName)}.
 */
@EnterprisePolicy(deviceOwner = GLOBAL, profileOwner = NO)
public final class UserControlDisabledPackages {
}
