/*
 * Copyright (C) 2020 The Android Open Source Project
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

@file:JvmName("AutoRevokePermissions")

package com.android.permissioncontroller.permission.service

import android.content.Context
import android.content.pm.PackageManager.FLAG_PERMISSION_AUTO_REVOKED
import android.content.pm.PackageManager.FLAG_PERMISSION_USER_SET
import android.os.UserHandle
import android.os.UserManager
import androidx.annotation.MainThread
import com.android.permissioncontroller.Constants.INVALID_SESSION_ID
import com.android.permissioncontroller.DumpableLog
import com.android.permissioncontroller.PermissionControllerStatsLog
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_UNUSED_APP_PERMISSION_REVOKED
import com.android.permissioncontroller.permission.data.LightAppPermGroupLiveData
import com.android.permissioncontroller.permission.data.PackagePermissionsLiveData
import com.android.permissioncontroller.permission.data.get
import com.android.permissioncontroller.permission.model.livedatatypes.LightAppPermGroup
import com.android.permissioncontroller.permission.model.livedatatypes.LightPackageInfo
import com.android.permissioncontroller.permission.utils.KotlinUtils
import com.android.permissioncontroller.permission.utils.Utils
import com.android.permissioncontroller.permission.utils.application
import com.android.permissioncontroller.permission.utils.forEachInParallel
import com.android.permissioncontroller.permission.utils.updatePermissionFlags
import kotlinx.coroutines.Dispatchers.Main
import java.util.concurrent.atomic.AtomicBoolean

private const val LOG_TAG = "AutoRevokePermissions"
const val DEBUG_AUTO_REVOKE = true

private val EXEMPT_PERMISSIONS = listOf(
        android.Manifest.permission.ACTIVITY_RECOGNITION)

private val SERVER_LOG_ID =
    PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_UNUSED_APP_PERMISSION_REVOKED

/**
 * Revoke granted app permissions for apps that should be auto-revoked
 *
 * @return list of packages that successfully had their permissions revoked
 */
@MainThread
suspend fun revokeAppPermissions(
    apps: Map<UserHandle, List<LightPackageInfo>>,
    context: Context,
    sessionId: Long = INVALID_SESSION_ID
): Set<Pair<String, UserHandle>> {
    val revokedApps = mutableSetOf<Pair<String, UserHandle>>()
    val userManager = context.getSystemService(UserManager::class.java)

    for ((user, userApps) in apps) {
        if (userManager == null || !userManager.isUserUnlocked(user)) {
            DumpableLog.w(LOG_TAG, "Skipping $user - locked direct boot state")
            continue
        }
        userApps.forEachInParallel(Main) { pkg: LightPackageInfo ->
            if (pkg.grantedPermissions.isEmpty()) {
                return@forEachInParallel
            }
            val packageName = pkg.packageName
            val anyPermsRevoked = AtomicBoolean(false)
            val pkgPermGroups: Map<String, List<String>>? =
                PackagePermissionsLiveData[packageName, user]
                    .getInitializedValue()

            pkgPermGroups?.entries?.forEachInParallel(Main) { (groupName, _) ->
                if (groupName == PackagePermissionsLiveData.NON_RUNTIME_NORMAL_PERMS ||
                    groupName !in Utils.getPlatformPermissionGroups()) {
                    return@forEachInParallel
                }

                val group: LightAppPermGroup =
                    LightAppPermGroupLiveData[packageName, groupName, user]
                        .getInitializedValue()
                        ?: return@forEachInParallel

                val fixed = group.isBackgroundFixed || group.isForegroundFixed
                val granted = group.permissions.any { (_, perm) ->
                    perm.isGrantedIncludingAppOp && perm.name !in EXEMPT_PERMISSIONS
                }
                if (!fixed &&
                    granted &&
                    !group.isGrantedByDefault &&
                    !group.isGrantedByRole &&
                    !group.isRevokeWhenRequested &&
                    group.isUserSensitive) {

                    val revocablePermissions = group.permissions.keys.toList()

                    if (revocablePermissions.isEmpty()) {
                        return@forEachInParallel
                    }

                    if (DEBUG_AUTO_REVOKE) {
                        DumpableLog.i(LOG_TAG,
                                "revokeUnused $packageName - $revocablePermissions")
                    }

                    val uid = group.packageInfo.uid
                    for (permName in revocablePermissions) {
                        PermissionControllerStatsLog.write(
                            PERMISSION_GRANT_REQUEST_RESULT_REPORTED,
                            sessionId, uid, packageName, permName, false, SERVER_LOG_ID)
                    }

                    if (DEBUG_AUTO_REVOKE) {
                        DumpableLog.i(LOG_TAG, "revoking $packageName - $revocablePermissions")
                        DumpableLog.i(LOG_TAG, "State pre revocation: ${group.allPermissions}")
                    }
                    anyPermsRevoked.compareAndSet(false, true)

                    val bgRevokedState = KotlinUtils.revokeBackgroundRuntimePermissions(
                            context.application, group,
                            userFixed = false, oneTime = false,
                            filterPermissions = revocablePermissions)
                    if (DEBUG_AUTO_REVOKE) {
                        DumpableLog.i(LOG_TAG,
                            "Bg state post revocation: ${bgRevokedState.allPermissions}")
                    }
                    val fgRevokedState = KotlinUtils.revokeForegroundRuntimePermissions(
                        context.application, group,
                        userFixed = false, oneTime = false,
                        filterPermissions = revocablePermissions)
                    if (DEBUG_AUTO_REVOKE) {
                        DumpableLog.i(LOG_TAG,
                            "Fg state post revocation: ${fgRevokedState.allPermissions}")
                    }

                    for (permission in revocablePermissions) {
                        context.packageManager.updatePermissionFlags(
                            permission, packageName, user,
                            FLAG_PERMISSION_AUTO_REVOKED to true,
                            FLAG_PERMISSION_USER_SET to false)
                    }
                }
            }

            if (anyPermsRevoked.get()) {
                synchronized(revokedApps) {
                    revokedApps.add(packageName to user)
                }
            }
        }
        if (DEBUG_AUTO_REVOKE) {
            synchronized(revokedApps) {
                DumpableLog.i(LOG_TAG,
                        "Done auto-revoke for user ${user.identifier} - revoked $revokedApps")
            }
        }
    }
    return revokedApps
}
