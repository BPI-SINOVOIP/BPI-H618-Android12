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

package com.android.cts.packagemanager.verify.domain.device.multiuser

import android.content.Context
import android.os.UserManager
import com.android.bedstead.harrier.DeviceState
import com.android.bedstead.nene.TestApis
import com.android.bedstead.nene.users.UserReference
import com.android.bedstead.remotedpc.managers.RemoteDevicePolicyManager

internal fun RemoteDevicePolicyManager.getAppLinkPolicy() =
    getUserRestrictions()?.getBoolean(UserManager.ALLOW_PARENT_PROFILE_APP_LINKING, false) ?: false

internal fun RemoteDevicePolicyManager.setAppLinkPolicy(allow: Boolean) {
    if (allow) {
        addUserRestriction(UserManager.ALLOW_PARENT_PROFILE_APP_LINKING)
    } else {
        clearUserRestriction(UserManager.ALLOW_PARENT_PROFILE_APP_LINKING)
    }
}

internal fun DeviceState.getWorkDevicePolicyManager() =
    profileOwner(workProfile(DeviceState.UserType.PRIMARY_USER))!!.devicePolicyManager()

internal fun <T> TestApis.withUserContext(user: UserReference, block: (context: Context) -> T) =
    permissions()
        .withPermission("android.permission.INTERACT_ACROSS_USERS_FULL")
        .use { block(context().androidContextAsUser(user)) }
