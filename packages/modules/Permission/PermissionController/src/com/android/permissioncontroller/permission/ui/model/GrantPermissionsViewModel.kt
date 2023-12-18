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

package com.android.permissioncontroller.permission.ui.model

import android.Manifest
import android.Manifest.permission.ACCESS_COARSE_LOCATION
import android.Manifest.permission.ACCESS_FINE_LOCATION
import android.Manifest.permission_group.LOCATION
import android.app.Activity
import android.app.Application
import android.app.admin.DevicePolicyManager
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.PackageManager.FLAG_PERMISSION_POLICY_FIXED
import android.content.pm.PackageManager.FLAG_PERMISSION_USER_FIXED
import android.content.pm.PackageManager.FLAG_PERMISSION_USER_SET
import android.os.Build
import android.os.Bundle
import android.os.Process
import android.permission.PermissionManager
import android.util.Log
import androidx.core.util.Consumer
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.android.permissioncontroller.Constants
import com.android.permissioncontroller.PermissionControllerStatsLog
import com.android.permissioncontroller.PermissionControllerStatsLog.GRANT_PERMISSIONS_ACTIVITY_BUTTON_ACTIONS
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_DENIED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_GRANTED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED_POLICY_FIXED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED_RESTRICTED_PERMISSION
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED_USER_FIXED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_IN_SETTINGS
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_WITH_PREJUDICE
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_WITH_PREJUDICE_IN_SETTINGS
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED_IN_SETTINGS
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED_ONE_TIME
import com.android.permissioncontroller.PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_IGNORED
import com.android.permissioncontroller.permission.data.LightAppPermGroupLiveData
import com.android.permissioncontroller.permission.data.LightPackageInfoLiveData
import com.android.permissioncontroller.permission.data.PackagePermissionsLiveData
import com.android.permissioncontroller.permission.data.SmartUpdateMediatorLiveData
import com.android.permissioncontroller.permission.data.get
import com.android.permissioncontroller.permission.model.livedatatypes.LightAppPermGroup
import com.android.permissioncontroller.permission.model.livedatatypes.LightPackageInfo
import com.android.permissioncontroller.permission.model.livedatatypes.LightPermGroupInfo
import com.android.permissioncontroller.permission.ui.AutoGrantPermissionsNotifier
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.ALLOW_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.ALLOW_FOREGROUND_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.ALLOW_ONE_TIME_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.COARSE_RADIO_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.DENY_AND_DONT_ASK_AGAIN_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.DENY_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.DIALOG_WITH_BOTH_LOCATIONS
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.DIALOG_WITH_COARSE_LOCATION_ONLY
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.DIALOG_WITH_FINE_LOCATION_ONLY
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.FINE_RADIO_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.LINK_TO_SETTINGS
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.LOCATION_ACCURACY_LAYOUT
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.NEXT_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.NEXT_LOCATION_DIALOG
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.NO_UPGRADE_AND_DONT_ASK_AGAIN_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.NO_UPGRADE_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.NO_UPGRADE_OT_AND_DONT_ASK_AGAIN_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.NO_UPGRADE_OT_BUTTON
import com.android.permissioncontroller.permission.ui.GrantPermissionsActivity.PERMISSION_TO_BIT_SHIFT
import com.android.permissioncontroller.permission.ui.GrantPermissionsViewHandler
import com.android.permissioncontroller.permission.ui.GrantPermissionsViewHandler.DENIED
import com.android.permissioncontroller.permission.ui.GrantPermissionsViewHandler.DENIED_DO_NOT_ASK_AGAIN
import com.android.permissioncontroller.permission.ui.GrantPermissionsViewHandler.GRANTED_ALWAYS
import com.android.permissioncontroller.permission.ui.GrantPermissionsViewHandler.GRANTED_FOREGROUND_ONLY
import com.android.permissioncontroller.permission.ui.handheld.dashboard.getDefaultPrecision
import com.android.permissioncontroller.permission.ui.handheld.dashboard.isLocationAccuracyEnabled
import com.android.permissioncontroller.permission.ui.ManagePermissionsActivity
import com.android.permissioncontroller.permission.ui.ManagePermissionsActivity.EXTRA_RESULT_PERMISSION_INTERACTED
import com.android.permissioncontroller.permission.ui.ManagePermissionsActivity.EXTRA_RESULT_PERMISSION_RESULT
import com.android.permissioncontroller.permission.utils.AdminRestrictedPermissionsUtils
import com.android.permissioncontroller.permission.utils.KotlinUtils
import com.android.permissioncontroller.permission.utils.SafetyNetLogger
import com.android.permissioncontroller.permission.utils.Utils
import kotlinx.coroutines.Dispatchers.Main
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch

/**
 * ViewModel for the GrantPermissionsActivity. Tracks all permission groups that are affected by
 * the permissions requested by the user, and generates a RequestInfo object for each group, if
 * action is needed. It will not return any data if one of the requests is malformed.
 *
 * @param app: The current application
 * @param packageName: The packageName permissions are being requested for
 * @param requestedPermissions: The list of permissions requested
 * @param sessionId: A long to identify this session
 * @param storedState: Previous state, if this activity was stopped and is being recreated
 */
class GrantPermissionsViewModel(
    private val app: Application,
    private val packageName: String,
    private val requestedPermissions: List<String>,
    private val sessionId: Long,
    private val storedState: Bundle?
) : ViewModel() {
    private val LOG_TAG = GrantPermissionsViewModel::class.java.simpleName
    private val user = Process.myUserHandle()
    private val packageInfoLiveData = LightPackageInfoLiveData[packageName, user]
    private val dpm = app.getSystemService(DevicePolicyManager::class.java)!!
    private val permissionPolicy = dpm.getPermissionPolicy(null)
    private val permGroupsToSkip = mutableListOf<String>()
    private var groupStates = mutableMapOf<Pair<String, Boolean>, GroupState>()
    private var isFirstTimeRequestingFineAndCoarse: Boolean = false

    private var autoGrantNotifier: AutoGrantPermissionsNotifier? = null
    private fun getAutoGrantNotifier(): AutoGrantPermissionsNotifier {
        autoGrantNotifier = AutoGrantPermissionsNotifier(app, packageInfo.toPackageInfo(app)!!)
        return autoGrantNotifier!!
    }

    private lateinit var packageInfo: LightPackageInfo

    // All permissions that could possibly be affected by the provided requested permissions, before
    // filtering system fixed, auto grant, etc.
    private var unfilteredAffectedPermissions = requestedPermissions

    /**
     * A class which represents a correctly requested permission group, and the buttons and messages
     * which should be shown with it.
     */
    data class RequestInfo(
        val groupInfo: LightPermGroupInfo,
        val buttonVisibilities: List<Boolean> = List(NEXT_BUTTON) { false },
        val locationVisibilities: List<Boolean> = List(NEXT_LOCATION_DIALOG) { false },
        val message: RequestMessage = RequestMessage.FG_MESSAGE,
        val detailMessage: RequestMessage = RequestMessage.NO_MESSAGE,
        val sendToSettingsImmediately: Boolean = false
    ) {
        val groupName = groupInfo.name
    }

    var activityResultCallback: Consumer<Intent>? = null

    /**
     * A LiveData which holds a list of the currently pending RequestInfos
     */
    val requestInfosLiveData = object :
        SmartUpdateMediatorLiveData<List<RequestInfo>>() {
        private val LOG_TAG = GrantPermissionsViewModel::class.java.simpleName
        private val packagePermissionsLiveData = PackagePermissionsLiveData[packageName, user]
        private val appPermGroupLiveDatas = mutableMapOf<String, LightAppPermGroupLiveData>()

        init {
            GlobalScope.launch(Main.immediate) {
                val groups = packagePermissionsLiveData.getInitializedValue()
                if (groups == null || groups.isEmpty()) {
                    Log.e(LOG_TAG, "Package $packageName not found")
                    value = null
                    return@launch
                }
                packageInfo = packageInfoLiveData.getInitializedValue()

                if (packageInfo.requestedPermissions.isEmpty() ||
                    packageInfo.targetSdkVersion < Build.VERSION_CODES.M) {
                    Log.e(LOG_TAG, "Package $packageName has no requested permissions, or " +
                        "is a pre-M app")
                    value = null
                    return@launch
                }

                val allAffectedPermissions = requestedPermissions.toMutableSet()
                for (requestedPerm in requestedPermissions) {
                    allAffectedPermissions.addAll(computeAffectedPermissions(requestedPerm, groups))
                }
                unfilteredAffectedPermissions = allAffectedPermissions.toList()

                getAppPermGroups(groups.toMutableMap().apply {
                        remove(PackagePermissionsLiveData.NON_RUNTIME_NORMAL_PERMS)
                    })
            }
        }

        private fun getAppPermGroups(groups: Map<String, List<String>>) {

            val requestedGroups = groups.filter { (_, perms) ->
                perms.any { it in unfilteredAffectedPermissions }
            }

            if (requestedGroups.isEmpty()) {
                Log.e(LOG_TAG, "None of " +
                    "$unfilteredAffectedPermissions in $groups")
                value = null
                return
            }

            val getLiveDataFun = { groupName: String ->
                LightAppPermGroupLiveData[packageName, groupName, user]
            }
            setSourcesToDifference(requestedGroups.keys, appPermGroupLiveDatas, getLiveDataFun)
        }

        override fun onUpdate() {
            if (appPermGroupLiveDatas.any { it.value.isStale }) {
                return
            }
            var newGroups = false
            for ((groupName, groupLiveData) in appPermGroupLiveDatas) {
                val appPermGroup = groupLiveData.value
                if (appPermGroup == null || groupName in permGroupsToSkip) {
                    if (appPermGroup == null) {
                        Log.e(LOG_TAG, "Group $packageName $groupName invalid")
                    }
                    groupStates[groupName to true]?.state = STATE_SKIPPED
                    groupStates[groupName to false]?.state = STATE_SKIPPED
                    continue
                }

                packageInfo = appPermGroup.packageInfo

                val states = groupStates.filter { it.key.first == groupName }
                if (states.isNotEmpty()) {
                    // some requests might have been granted, check for that
                    for ((key, state) in states) {
                        val allAffectedGranted = state.affectedPermissions.all { perm ->
                            appPermGroup.permissions[perm]?.isGrantedIncludingAppOp == true
                        }
                        if (allAffectedGranted) {
                            groupStates[key]!!.state = STATE_ALLOWED
                        }
                    }
                } else {
                    newGroups = true
                }
            }

            if (newGroups) {
                groupStates = getRequiredGroupStates(
                    appPermGroupLiveDatas.mapNotNull { it.value.value })
            }
            getRequestInfosFromGroupStates()
        }

        private fun getRequestInfosFromGroupStates() {
            val requestInfos = mutableListOf<RequestInfo>()
            for ((key, groupState) in groupStates) {
                val groupInfo = groupState.group.permGroupInfo
                val (groupName, isBackground) = key
                if (groupState.state != STATE_UNKNOWN) {
                    continue
                }

                val fgState = groupStates[groupName to false]
                val bgState = groupStates[groupName to true]
                var needFgPermissions = false
                var needBgPermissions = false
                var isFgUserSet = false
                var isBgUserSet = false

                if (fgState?.group != null) {
                    val fgGroup = fgState.group
                    for (perm in fgState.affectedPermissions) {
                        if (fgGroup.permissions[perm]?.isGrantedIncludingAppOp == false) {
                            // If any of the requested permissions is not granted,
                            // needFgPermissions = true
                            needFgPermissions = true
                            // If any of the requested permission's UserSet is true and the
                            // permission is not granted, isFgUserSet = true.
                            if (fgGroup.permissions[perm]?.isUserSet == true) {
                                isFgUserSet = true
                            }
                        }
                    }
                }

                if (bgState?.group?.background?.isGranted == false) {
                    needBgPermissions = true
                    isBgUserSet = bgState.group.background.isUserSet
                }

                val buttonVisibilities = MutableList(NEXT_BUTTON) { false }
                buttonVisibilities[ALLOW_BUTTON] = true
                buttonVisibilities[DENY_BUTTON] = true
                buttonVisibilities[ALLOW_ONE_TIME_BUTTON] =
                    Utils.supportsOneTimeGrant(groupName)
                var message = RequestMessage.FG_MESSAGE
                // Whether or not to use the foreground, background, or no detail message.
                // null ==
                var detailMessage = RequestMessage.NO_MESSAGE

                if (groupState.group.packageInfo.targetSdkVersion >= Build.VERSION_CODES.R) {
                    if (isBackground || groupState.group.hasPermWithBackgroundMode) {
                        if (needFgPermissions) {
                            if (needBgPermissions) {
                                if (groupState.group.permGroupName
                                                .equals(Manifest.permission_group.CAMERA) ||
                                        groupState.group.permGroupName
                                                .equals(Manifest.permission_group.MICROPHONE)) {
                                    if (groupState.group.packageInfo.targetSdkVersion >=
                                            Build.VERSION_CODES.S) {
                                        Log.e(LOG_TAG,
                                                "For S apps, background permissions must be " +
                                                "requested after foreground permissions are" +
                                                        " already granted")
                                        value = null
                                        return
                                    } else {
                                        // Case: sdk < S, BG&FG mic/camera permission requested
                                        buttonVisibilities[ALLOW_BUTTON] = false
                                        buttonVisibilities[ALLOW_FOREGROUND_BUTTON] = true
                                        buttonVisibilities[DENY_BUTTON] = !isFgUserSet
                                        buttonVisibilities[DENY_AND_DONT_ASK_AGAIN_BUTTON] =
                                                isFgUserSet
                                        if (needBgPermissions) {
                                            // Case: sdk < R, BG/FG permission requesting both
                                            message = RequestMessage.BG_MESSAGE
                                            detailMessage = RequestMessage.BG_MESSAGE
                                        }
                                    }
                                } else {
                                    // Shouldn't be reached as background must be requested as a
                                    // singleton
                                    Log.e(LOG_TAG, "For R+ apps, background permissions must be " +
                                            "requested after foreground permissions are already" +
                                            " granted")
                                    value = null
                                    return
                                }
                            } else {
                                buttonVisibilities[ALLOW_BUTTON] = false
                                buttonVisibilities[ALLOW_FOREGROUND_BUTTON] = true
                                buttonVisibilities[DENY_BUTTON] = !isFgUserSet
                                buttonVisibilities[DENY_AND_DONT_ASK_AGAIN_BUTTON] = isFgUserSet
                            }
                        } else if (needBgPermissions) {
                            // Case: sdk >= R, BG/FG permission requesting BG only
                            requestInfos.add(RequestInfo(
                                groupInfo, sendToSettingsImmediately = true))
                            continue
                        } else {
                            // Not reached as the permissions should be auto-granted
                            value = null
                            return
                        }
                    } else {
                        // Case: sdk >= R, Requesting normal permission
                        buttonVisibilities[DENY_BUTTON] = !isFgUserSet
                        buttonVisibilities[DENY_AND_DONT_ASK_AGAIN_BUTTON] = isFgUserSet
                    }
                } else {
                    if (isBackground || groupState.group.hasPermWithBackgroundMode) {
                        if (needFgPermissions) {
                            // Case: sdk < R, BG/FG permission requesting both or FG only
                            buttonVisibilities[ALLOW_BUTTON] = false
                            buttonVisibilities[ALLOW_FOREGROUND_BUTTON] = true
                            buttonVisibilities[DENY_BUTTON] = !isFgUserSet
                            buttonVisibilities[DENY_AND_DONT_ASK_AGAIN_BUTTON] = isFgUserSet
                            if (needBgPermissions) {
                                // Case: sdk < R, BG/FG permission requesting both
                                message = RequestMessage.BG_MESSAGE
                                detailMessage = RequestMessage.BG_MESSAGE
                            }
                        } else if (needBgPermissions) {
                            // Case: sdk < R, BG/FG permission requesting BG only
                            if (!groupState.group.foreground.isGranted) {
                                Log.e(LOG_TAG, "Background permissions can't be requested " +
                                        "solely before foreground permissions are granted.")
                                value = null
                                return
                            }
                            message = RequestMessage.UPGRADE_MESSAGE
                            detailMessage = RequestMessage.UPGRADE_MESSAGE
                            buttonVisibilities[ALLOW_BUTTON] = false
                            buttonVisibilities[DENY_BUTTON] = false
                            buttonVisibilities[ALLOW_ONE_TIME_BUTTON] = false
                            if (groupState.group.isOneTime) {
                                buttonVisibilities[NO_UPGRADE_OT_BUTTON] = !isBgUserSet
                                buttonVisibilities[NO_UPGRADE_OT_AND_DONT_ASK_AGAIN_BUTTON] =
                                    isBgUserSet
                            } else {
                                buttonVisibilities[NO_UPGRADE_BUTTON] = !isBgUserSet
                                buttonVisibilities[NO_UPGRADE_AND_DONT_ASK_AGAIN_BUTTON] =
                                    isBgUserSet
                            }
                        } else {
                            // Not reached as the permissions should be auto-granted
                            value = null
                            return
                        }
                    } else {
                        // If no permissions needed, do nothing
                        if (!needFgPermissions && !needBgPermissions) {
                            value = null
                            return
                        }
                        // Case: sdk < R, Requesting normal permission
                        buttonVisibilities[DENY_BUTTON] = !isFgUserSet
                        buttonVisibilities[DENY_AND_DONT_ASK_AGAIN_BUTTON] = isFgUserSet
                    }
                }
                buttonVisibilities[LINK_TO_SETTINGS] =
                    detailMessage != RequestMessage.NO_MESSAGE

                // Show location permission dialogs based on location permissions
                val locationVisibilities = MutableList(NEXT_LOCATION_DIALOG) { false }
                if (groupState.group.permGroupName == LOCATION && isLocationAccuracyEnabled() &&
                        packageInfo.targetSdkVersion >= Build.VERSION_CODES.S) {
                    if (needFgPermissions) {
                        locationVisibilities[LOCATION_ACCURACY_LAYOUT] = true
                        if (fgState != null &&
                                fgState.affectedPermissions.contains(ACCESS_FINE_LOCATION)) {
                            val coarseLocationPerm =
                                groupState.group.allPermissions[ACCESS_COARSE_LOCATION]
                            if (coarseLocationPerm?.isGrantedIncludingAppOp == true) {
                                // Upgrade flow
                                locationVisibilities[DIALOG_WITH_FINE_LOCATION_ONLY] = true
                                message = RequestMessage.FG_FINE_LOCATION_MESSAGE
                                // If COARSE was granted one time, hide 'While in use' button
                                if (coarseLocationPerm.isOneTime) {
                                    buttonVisibilities[ALLOW_FOREGROUND_BUTTON] = false
                                }
                            } else {
                                if (!fgState.affectedPermissions.contains(ACCESS_COARSE_LOCATION)) {
                                    Log.e(LOG_TAG, "ACCESS_FINE_LOCATION must be requested " +
                                            "with ACCESS_COARSE_LOCATION.")
                                    value = null
                                    return
                                }
                                if (coarseLocationPerm?.isOneTime == false &&
                                        !coarseLocationPerm.isUserSet &&
                                        !coarseLocationPerm.isUserFixed) {
                                    isFirstTimeRequestingFineAndCoarse = true
                                }
                                // Normal flow with both Coarse and Fine locations
                                locationVisibilities[DIALOG_WITH_BOTH_LOCATIONS] = true
                                // Steps to decide location accuracy default state
                                // 1. If none of the FINE and COARSE isSelectedLocationAccuracy
                                //    flags is set, then use default precision from device config.
                                // 2. Otherwise set to whichever isSelectedLocationAccuracy is true.
                                val fineLocationPerm =
                                        groupState.group.allPermissions[ACCESS_FINE_LOCATION]
                                if (coarseLocationPerm?.isSelectedLocationAccuracy == false &&
                                        fineLocationPerm?.isSelectedLocationAccuracy == false) {
                                    if (getDefaultPrecision()) {
                                        locationVisibilities[FINE_RADIO_BUTTON] = true
                                    } else {
                                        locationVisibilities[COARSE_RADIO_BUTTON] = true
                                    }
                                } else if (coarseLocationPerm?.isSelectedLocationAccuracy == true) {
                                    locationVisibilities[COARSE_RADIO_BUTTON] = true
                                } else {
                                    locationVisibilities[FINE_RADIO_BUTTON] = true
                                }
                            }
                        } else if (fgState != null && fgState.affectedPermissions
                                        .contains(ACCESS_COARSE_LOCATION)) {
                            // Request Coarse only
                            locationVisibilities[DIALOG_WITH_COARSE_LOCATION_ONLY] = true
                            message = RequestMessage.FG_COARSE_LOCATION_MESSAGE
                        }
                    }
                }

                requestInfos.add(RequestInfo(
                    groupInfo,
                    buttonVisibilities,
                    locationVisibilities,
                    message,
                    detailMessage))
            }
            requestInfos.sortWith(Comparator { rhs, lhs ->
                val rhsHasOneTime = rhs.buttonVisibilities[ALLOW_ONE_TIME_BUTTON]
                val lhsHasOneTime = lhs.buttonVisibilities[ALLOW_ONE_TIME_BUTTON]
                if (rhsHasOneTime && !lhsHasOneTime) {
                    -1
                } else if (!rhsHasOneTime && lhsHasOneTime) {
                    1
                } else {
                    rhs.groupName.compareTo(lhs.groupName)
                }
            })

            value = if (requestInfos.any { it.sendToSettingsImmediately } &&
                requestInfos.size > 1) {
                Log.e(LOG_TAG, "For R+ apps, background permissions must be requested " +
                    "individually")
                null
            } else {
                requestInfos
            }
        }
    }

    /**
     * Converts a list of LightAppPermGroups into a list of GroupStates
     */
    private fun getRequiredGroupStates(
        groups: List<LightAppPermGroup>
    ): MutableMap<Pair<String, Boolean>, GroupState> {
        val groupStates = mutableMapOf<Pair<String, Boolean>, GroupState>()
        val filteredPermissions = unfilteredAffectedPermissions.filter { perm ->
            val group = getGroupWithPerm(perm, groups)
            group != null && isPermissionGrantableAndNotFixed(perm, group)
        }
        for (perm in filteredPermissions) {
            val group = getGroupWithPerm(perm, groups)!!

            val isBackground = perm in group.backgroundPermNames
            val groupStateInfo = groupStates.getOrPut(group.permGroupName to isBackground) {
                GroupState(group, isBackground)
            }

            var currGroupState = groupStateInfo.state
            if (storedState != null && currGroupState != STATE_UNKNOWN) {
                currGroupState = storedState.getInt(getInstanceStateKey(group.permGroupName,
                    isBackground), STATE_UNKNOWN)
            }

            val otherGroupPermissions = filteredPermissions.filter { it in group.permissions }
            val groupStateOfPerm = getGroupState(perm, group, otherGroupPermissions)
            if (groupStateOfPerm != STATE_UNKNOWN) {
                currGroupState = groupStateOfPerm
            }

            if (group.permGroupName in permGroupsToSkip) {
                currGroupState = STATE_SKIPPED
            }

            if (currGroupState != STATE_UNKNOWN) {
                groupStateInfo.state = currGroupState
            }
            // If we saved state, load it
            groupStateInfo.affectedPermissions.add(perm)
        }
        return groupStates
    }

    /**
     * Get the actually requested permissions when a permission is requested.
     *
     * >In some cases requesting to grant a single permission requires the system to grant
     * additional permissions. E.g. before N-MR1 a single permission of a group caused the whole
     * group to be granted. Another case are permissions that are split into two. For apps that
     * target an SDK before the split, this method automatically adds the split off permission.
     *
     * @param perm The requested permission
     *
     * @return The actually requested permissions
     */
    private fun computeAffectedPermissions(
        perm: String,
        appPermissions: Map<String, List<String>>
    ): List<String> {
        val requestingAppTargetSDK = packageInfo.targetSdkVersion

        // If a permission is split, all permissions the original permission is split into are
        // affected
        val extendedBySplitPerms = mutableListOf(perm)

        val splitPerms = app.getSystemService(PermissionManager::class.java)!!.splitPermissions
        for (splitPerm in splitPerms) {

            if (requestingAppTargetSDK < splitPerm.targetSdk && perm == splitPerm.splitPermission) {
                extendedBySplitPerms.addAll(splitPerm.newPermissions)
            }
        }

        // For <= N_MR1 apps all permissions of the groups of the requested permissions are affected
        if (requestingAppTargetSDK <= Build.VERSION_CODES.N_MR1) {
            val extendedBySplitPermsAndGroup = mutableListOf<String>()

            for (splitPerm in extendedBySplitPerms) {
                val groups = appPermissions.filter { splitPerm in it.value }
                if (groups.isEmpty()) {
                    continue
                }

                val permissionsInGroup = groups.values.first()
                for (permissionInGroup in permissionsInGroup) {
                    extendedBySplitPermsAndGroup.add(permissionInGroup)
                }
            }

            return extendedBySplitPermsAndGroup
        } else {
            return extendedBySplitPerms
        }
    }

    private fun isPermissionGrantableAndNotFixed(perm: String, group: LightAppPermGroup): Boolean {

        // If the permission is restricted it does not show in the UI and
        // is not added to the group at all, so check that first.
        if (perm in group.packageInfo.requestedPermissions && perm !in group.permissions) {
            reportRequestResult(perm,
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED_RESTRICTED_PERMISSION)
            return false
        }

        val subGroup = if (perm in group.backgroundPermNames) {
            group.background
        } else {
            group.foreground
        }

        val lightPermission = group.permissions[perm] ?: return false

        if (!subGroup.isGrantable) {
            reportRequestResult(perm, PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED)
            // Skip showing groups that we know cannot be granted.
            return false
        } else if (subGroup.isUserFixed) {
            reportRequestResult(perm,
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED_USER_FIXED)
            return false
        } else if (subGroup.isPolicyFixed && !subGroup.isGranted || lightPermission.isPolicyFixed) {
            reportRequestResult(perm,
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED_POLICY_FIXED)
            return false
        }

        return true
    }

    private fun getGroupState(
        perm: String,
        group: LightAppPermGroup,
        groupRequestedPermissions: List<String>
    ): Int {
        val policyState = getStateFromPolicy(perm, group)
        if (policyState != STATE_UNKNOWN) {
            return policyState
        }

        val isBackground = perm in group.backgroundPermNames

        val hasForegroundRequest = groupRequestedPermissions.any {
            it !in group.backgroundPermNames
        }

        // Do not attempt to grant background access if foreground access is not either already
        // granted or requested
        if (isBackground && !group.foreground.isGranted && !hasForegroundRequest) {
            Log.w(LOG_TAG, "Cannot grant $perm as the matching foreground permission is not " +
                "already granted.")
            val affectedPermissions = groupRequestedPermissions.filter {
                it in group.backgroundPermNames
            }
            reportRequestResult(affectedPermissions,
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__IGNORED)
            return STATE_SKIPPED
        }

        if (isBackground && group.background.isGranted ||
            !isBackground && group.foreground.isGranted) {
            // If FINE location is not granted, do not grant it automatically when COARSE
            // location is already granted.
            if (group.permGroupName == LOCATION &&
                    group.allPermissions[ACCESS_FINE_LOCATION]?.isGrantedIncludingAppOp
                    == false) {
                return STATE_UNKNOWN
            }

            if (group.permissions[perm]?.isGrantedIncludingAppOp == false) {
                if (isBackground) {
                    KotlinUtils.grantBackgroundRuntimePermissions(app, group, listOf(perm))
                } else {
                    KotlinUtils.grantForegroundRuntimePermissions(app, group, listOf(perm))
                }
                KotlinUtils.setGroupFlags(app, group, FLAG_PERMISSION_USER_SET to false,
                    FLAG_PERMISSION_USER_FIXED to false, filterPermissions = listOf(perm))
                reportRequestResult(perm,
                    PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_GRANTED)
            }

            return if (storedState == null) {
                STATE_SKIPPED
            } else {
                STATE_ALLOWED
            }
        }
        return STATE_UNKNOWN
    }

    private fun getStateFromPolicy(perm: String, group: LightAppPermGroup): Int {
        val isBackground = perm in group.backgroundPermNames
        var skipGroup = false
        var state = STATE_UNKNOWN
        when (permissionPolicy) {
            DevicePolicyManager.PERMISSION_POLICY_AUTO_GRANT -> {
                if (AdminRestrictedPermissionsUtils.mayAdminGrantPermission(
                                app, perm, user.identifier)) {
                    if (isBackground) {
                        KotlinUtils.grantBackgroundRuntimePermissions(app, group, listOf(perm))
                    } else {
                        KotlinUtils.grantForegroundRuntimePermissions(app, group, listOf(perm))
                    }
                    KotlinUtils.setGroupFlags(app, group, FLAG_PERMISSION_POLICY_FIXED to true,
                            FLAG_PERMISSION_USER_SET to false, FLAG_PERMISSION_USER_FIXED to false,
                            filterPermissions = listOf(perm))
                    state = STATE_ALLOWED
                    skipGroup = true

                    getAutoGrantNotifier().onPermissionAutoGranted(perm)
                    reportRequestResult(perm,
                            PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_GRANTED)
                }
            }

            DevicePolicyManager.PERMISSION_POLICY_AUTO_DENY -> {
                if (group.permissions[perm]?.isPolicyFixed == false) {
                    KotlinUtils.setGroupFlags(app, group, FLAG_PERMISSION_POLICY_FIXED to true,
                        FLAG_PERMISSION_USER_SET to false, FLAG_PERMISSION_USER_FIXED to false,
                        filterPermissions = listOf(perm))
                }
                state = STATE_DENIED
                skipGroup = true

                reportRequestResult(perm,
                    PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__AUTO_DENIED)
            }
        }
        if (skipGroup && storedState == null) {
            return STATE_SKIPPED
        }
        return state
    }

    /**
     * Upon the user clicking a button, grant permissions, if applicable.
     *
     * @param groupName The name of the permission group which was changed
     * @param affectedForegroundPermissions The name of the foreground permission which was changed
     * @param result The choice the user made regarding the group.
     */
    fun onPermissionGrantResult(
        groupName: String?,
        affectedForegroundPermissions: List<String>?,
        result: Int
    ) {
        if (groupName == null) {
            return
        }
        val foregroundGroupState = groupStates[groupName to false]
        val backgroundGroupState = groupStates[groupName to true]
        when (result) {
            GrantPermissionsViewHandler.CANCELED -> {
                if (foregroundGroupState != null) {
                    reportRequestResult(foregroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_IGNORED)
                }
                if (backgroundGroupState != null) {
                    reportRequestResult(backgroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_IGNORED)
                }
                return
            }
            GRANTED_ALWAYS -> {
                if (foregroundGroupState != null) {
                    onPermissionGrantResultSingleState(foregroundGroupState,
                        affectedForegroundPermissions, granted = true, isOneTime = false,
                        doNotAskAgain = false)
                }
                if (backgroundGroupState != null) {
                    onPermissionGrantResultSingleState(backgroundGroupState,
                        affectedForegroundPermissions, granted = true, isOneTime = false,
                        doNotAskAgain = false)
                }
            }
            GRANTED_FOREGROUND_ONLY -> {
                if (foregroundGroupState != null) {
                    onPermissionGrantResultSingleState(foregroundGroupState,
                        affectedForegroundPermissions, granted = true, isOneTime = false,
                        doNotAskAgain = false)
                }
                if (backgroundGroupState != null) {
                    onPermissionGrantResultSingleState(backgroundGroupState,
                        affectedForegroundPermissions, granted = false, isOneTime = false,
                        doNotAskAgain = false)
                }
            }
            GrantPermissionsViewHandler.GRANTED_ONE_TIME -> {
                if (foregroundGroupState != null) {
                    onPermissionGrantResultSingleState(foregroundGroupState,
                        affectedForegroundPermissions, granted = true, isOneTime = true,
                        doNotAskAgain = false)
                }
                if (backgroundGroupState != null) {
                    onPermissionGrantResultSingleState(backgroundGroupState,
                        affectedForegroundPermissions, granted = false, isOneTime = true,
                        doNotAskAgain = false)
                }
            }
            DENIED -> {
                if (foregroundGroupState != null) {
                    onPermissionGrantResultSingleState(foregroundGroupState,
                        affectedForegroundPermissions, granted = false, isOneTime = false,
                        doNotAskAgain = false)
                }
                if (backgroundGroupState != null) {
                    onPermissionGrantResultSingleState(backgroundGroupState,
                        affectedForegroundPermissions, granted = false, isOneTime = false,
                        doNotAskAgain = false)
                }
            }
            DENIED_DO_NOT_ASK_AGAIN -> {
                if (foregroundGroupState != null) {
                    onPermissionGrantResultSingleState(foregroundGroupState,
                        affectedForegroundPermissions, granted = false, isOneTime = false,
                        doNotAskAgain = true)
                }
                if (backgroundGroupState != null) {
                    onPermissionGrantResultSingleState(backgroundGroupState,
                        affectedForegroundPermissions, granted = false, isOneTime = false,
                        doNotAskAgain = true)
                }
            }
        }
    }

    private fun onPermissionGrantResultSingleState(
        groupState: GroupState,
        affectedForegroundPermissions: List<String>?,
        granted: Boolean,
        isOneTime: Boolean,
        doNotAskAgain: Boolean
    ) {
        if (groupState.state != STATE_UNKNOWN) {
            // We already dealt with this group, don't re-grant/re-revoke
            return
        }
        val result: Int
        if (granted) {
            result = if (isOneTime) {
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED_ONE_TIME
            } else {
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED
            }
            if (groupState.isBackground) {
                KotlinUtils.grantBackgroundRuntimePermissions(app, groupState.group,
                    groupState.affectedPermissions)
            } else {
                if (affectedForegroundPermissions == null) {
                    KotlinUtils.grantForegroundRuntimePermissions(app, groupState.group,
                        groupState.affectedPermissions, isOneTime)
                    // This prevents weird flag state when app targetSDK switches from S+ to R-
                    if (groupState.affectedPermissions.contains(ACCESS_FINE_LOCATION)) {
                        KotlinUtils.setFlagsWhenLocationAccuracyChanged(
                                app, groupState.group, true)
                    }
                } else {
                    val newGroup = KotlinUtils.grantForegroundRuntimePermissions(app,
                            groupState.group, affectedForegroundPermissions, isOneTime)
                    if (!isOneTime || newGroup.isOneTime) {
                        KotlinUtils.setFlagsWhenLocationAccuracyChanged(app, newGroup,
                                affectedForegroundPermissions.contains(ACCESS_FINE_LOCATION))
                    }
                }
            }
            groupState.state = STATE_ALLOWED
        } else {
            if (groupState.isBackground) {
                KotlinUtils.revokeBackgroundRuntimePermissions(app, groupState.group,
                    userFixed = doNotAskAgain, filterPermissions = groupState.affectedPermissions)
            } else {
                if (affectedForegroundPermissions == null) {
                    KotlinUtils.revokeForegroundRuntimePermissions(app, groupState.group,
                        userFixed = doNotAskAgain,
                        filterPermissions = groupState.affectedPermissions, oneTime = isOneTime)
                } else {
                    KotlinUtils.revokeForegroundRuntimePermissions(app, groupState.group,
                        userFixed = doNotAskAgain,
                        filterPermissions = affectedForegroundPermissions, oneTime = isOneTime)
                }
            }
            result = if (doNotAskAgain) {
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_WITH_PREJUDICE
            } else {
                PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED
            }
            groupState.state = STATE_DENIED
        }
        reportRequestResult(groupState.affectedPermissions, result)
        // group state has changed, reload liveData
        requestInfosLiveData.update()
    }

    private fun getGroupWithPerm(
        perm: String,
        groups: List<LightAppPermGroup>
    ): LightAppPermGroup? {
        val groupsWithPerm = groups.filter { perm in it.permissions }
        if (groupsWithPerm.isEmpty()) {
            return null
        }
        return groupsWithPerm.first()
    }

    /**
     * An internal class which represents the state of a current AppPermissionGroup grant request.
     */
    internal class GroupState(
        internal val group: LightAppPermGroup,
        internal val isBackground: Boolean,
        internal val affectedPermissions: MutableList<String> = mutableListOf(),
        internal var state: Int = STATE_UNKNOWN
    ) {
        override fun toString(): String {
            val stateStr: String = when (state) {
                STATE_UNKNOWN -> "unknown"
                STATE_ALLOWED -> "granted"
                STATE_DENIED -> "denied"
                else -> "skipped"
            }
            return "${group.permGroupName} $isBackground $stateStr $affectedPermissions"
        }
    }

    private fun reportRequestResult(permissions: List<String>, result: Int) {
        for (perm in permissions) {
            reportRequestResult(perm, result)
        }
    }

    /**
     * Report the result of a grant of a permission.
     *
     * @param permission The permission that was granted or denied
     * @param result The permission grant result
     */
    private fun reportRequestResult(permission: String, result: Int) {
        val isImplicit = permission !in requestedPermissions

        Log.v(LOG_TAG, "Permission grant result requestId=$sessionId " +
            "callingUid=${packageInfo.uid} callingPackage=$packageName permission=$permission " +
            "isImplicit=$isImplicit result=$result")

        PermissionControllerStatsLog.write(
            PermissionControllerStatsLog.PERMISSION_GRANT_REQUEST_RESULT_REPORTED, sessionId,
            packageInfo.uid, packageName, permission, isImplicit, result)
    }

    /**
     * Save the group states of the view model, to allow for state restoration after lifecycle
     * events
     *
     * @param outState The bundle in which to store state
     */
    fun saveInstanceState(outState: Bundle) {
        for ((groupKey, groupState) in groupStates) {
            val (groupName, isBackground) = groupKey
            outState.putInt(getInstanceStateKey(groupName, isBackground), groupState.state)
        }
    }

    /**
     * Determine if the activity should return permission state to the caller
     *
     * @return Whether or not state should be returned. False only if the package is pre-M, true
     * otherwise.
     */
    fun shouldReturnPermissionState(): Boolean {
        return if (packageInfoLiveData.value != null) {
            packageInfoLiveData.value!!.targetSdkVersion >= Build.VERSION_CODES.M
        } else {
            // Should not be reached, as this method shouldn't be called before data is passed to
            // the activity for the first time
            try {
                Utils.getUserContext(app, user).packageManager
                    .getApplicationInfo(packageName, 0).targetSdkVersion >= Build.VERSION_CODES.M
            } catch (e: PackageManager.NameNotFoundException) {
                true
            }
        }
    }

    /**
     * Send the user directly to the AppPermissionFragment. Used for R+ apps.
     *
     * @param activity The current activity
     * @param groupName The name of the permission group whose fragment should be opened
     */
    fun sendDirectlyToSettings(activity: Activity, groupName: String) {
        if (activityResultCallback == null) {
            startAppPermissionFragment(activity, groupName)
            activityResultCallback = Consumer { data ->
                if (data?.getStringExtra(EXTRA_RESULT_PERMISSION_INTERACTED) == null) {
                    // User didn't interact, count against rate limit
                    val group = groupStates[groupName to false]?.group
                        ?: groupStates[groupName to true]?.group ?: return@Consumer
                    if (group.background.isUserSet) {
                        KotlinUtils.setGroupFlags(app, group, FLAG_PERMISSION_USER_FIXED to true,
                            filterPermissions = group.backgroundPermNames)
                    } else {
                        KotlinUtils.setGroupFlags(app, group, FLAG_PERMISSION_USER_SET to true,
                            filterPermissions = group.backgroundPermNames)
                    }
                }

                permGroupsToSkip.add(groupName)
                // Update our liveData now that there is a new skipped group
                requestInfosLiveData.update()
            }
        }
    }

    /**
     * Send the user to the AppPermissionFragment from a link. Used for Q- apps
     *
     * @param activity The current activity
     * @param groupName The name of the permission group whose fragment should be opened
     */
    fun sendToSettingsFromLink(activity: Activity, groupName: String) {
        startAppPermissionFragment(activity, groupName)
        activityResultCallback = Consumer { data ->
            val returnGroupName = data?.getStringExtra(EXTRA_RESULT_PERMISSION_INTERACTED)
            if (returnGroupName != null) {
                permGroupsToSkip.add(returnGroupName)
                val result = data.getIntExtra(EXTRA_RESULT_PERMISSION_RESULT, -1)
                logSettingsInteraction(returnGroupName, result)
                requestInfosLiveData.update()
            }
        }
    }

    private fun startAppPermissionFragment(activity: Activity, groupName: String) {
        val intent = Intent(Intent.ACTION_MANAGE_APP_PERMISSION)
            .putExtra(Intent.EXTRA_PACKAGE_NAME, packageName)
            .putExtra(Intent.EXTRA_PERMISSION_GROUP_NAME, groupName)
            .putExtra(Intent.EXTRA_USER, user)
            .putExtra(ManagePermissionsActivity.EXTRA_CALLER_NAME,
                GrantPermissionsActivity::class.java.name)
            .putExtra(Constants.EXTRA_SESSION_ID, sessionId)
            .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK)
        activity.startActivityForResult(intent, APP_PERMISSION_REQUEST_CODE)
    }

    private fun getInstanceStateKey(groupName: String, isBackground: Boolean): String {
        return "${this::class.java.name}_${groupName}_$isBackground"
    }

    private fun logSettingsInteraction(groupName: String, result: Int) {
        val foregroundGroupState = groupStates[groupName to false]
        val backgroundGroupState = groupStates[groupName to true]
        val deniedPrejudiceInSettings =
            PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_WITH_PREJUDICE_IN_SETTINGS
        when (result) {
            GRANTED_ALWAYS -> {
                if (foregroundGroupState != null) {
                    reportRequestResult(foregroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED_IN_SETTINGS)
                }
                if (backgroundGroupState != null) {
                    reportRequestResult(backgroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED_IN_SETTINGS)
                }
            }
            GRANTED_FOREGROUND_ONLY -> {
                if (foregroundGroupState != null) {
                    reportRequestResult(foregroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_GRANTED_IN_SETTINGS)
                }
                if (backgroundGroupState != null) {
                    reportRequestResult(backgroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_IN_SETTINGS)
                }
            }
            DENIED -> {
                if (foregroundGroupState != null) {
                    reportRequestResult(foregroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_IN_SETTINGS)
                }
                if (backgroundGroupState != null) {
                    reportRequestResult(backgroundGroupState.affectedPermissions,
                        PERMISSION_GRANT_REQUEST_RESULT_REPORTED__RESULT__USER_DENIED_IN_SETTINGS)
                }
            }
            DENIED_DO_NOT_ASK_AGAIN -> {
                if (foregroundGroupState != null) {
                    reportRequestResult(foregroundGroupState.affectedPermissions,
                        deniedPrejudiceInSettings)
                }
                if (backgroundGroupState != null) {
                    reportRequestResult(backgroundGroupState.affectedPermissions,
                        deniedPrejudiceInSettings)
                }
            }
        }
    }

    /**
     * Log all permission groups which were requested
     */
    fun logRequestedPermissionGroups() {
        if (groupStates.isEmpty()) {
            return
        }
        val groups = groupStates.map { it.value.group }
        SafetyNetLogger.logPermissionsRequested(packageName, packageInfo.uid, groups)
    }

    /**
     * Log information about the buttons which were shown and clicked by the user.
     *
     * @param groupName The name of the permission group which was interacted with
     * @param selectedPrecision Selected precision of the location permission - bit flags indicate
     *                          which locations were chosen
     * @param clickedButton The button that was clicked by the user
     * @param presentedButtons All buttons which were shown to the user
     */
    fun logClickedButtons(
        groupName: String?,
        selectedPrecision: Int,
        clickedButton: Int,
        presentedButtons: Int
    ) {
        if (groupName == null) {
            return
        }
        var selectedLocations = 0
        // log permissions if it's 1) first time requesting both locations OR 2) upgrade flow
        if (isFirstTimeRequestingFineAndCoarse ||
                selectedPrecision ==
                    1 shl PERMISSION_TO_BIT_SHIFT[ACCESS_FINE_LOCATION]!!) {
            selectedLocations = selectedPrecision
        }
        PermissionControllerStatsLog.write(GRANT_PERMISSIONS_ACTIVITY_BUTTON_ACTIONS,
                groupName, packageInfo.uid, packageName, presentedButtons, clickedButton, sessionId,
                packageInfo.targetSdkVersion, selectedLocations)
        Log.v(LOG_TAG, "Logged buttons presented and clicked permissionGroupName=" +
                "$groupName uid=${packageInfo.uid} selectedLocations=$selectedLocations " +
                "package=$packageName presentedButtons=$presentedButtons " +
                "clickedButton=$clickedButton sessionId=$sessionId " +
                "targetSdk=${packageInfo.targetSdkVersion}")
    }

    /**
     * Use the autoGrantNotifier to notify of auto-granted permissions.
     */
    fun autoGrantNotify() {
        autoGrantNotifier?.notifyOfAutoGrantPermissions(true)
    }

    companion object {
        private const val APP_PERMISSION_REQUEST_CODE = 1
        private const val STATE_UNKNOWN = 0
        private const val STATE_ALLOWED = 1
        private const val STATE_DENIED = 2
        private const val STATE_SKIPPED = 3
        private const val STATE_ALREADY_ALLOWED = 4

        /**
         * An enum that represents the type of message which should be shown- foreground,
         * background, upgrade, or no message.
         */
        enum class RequestMessage(request: Int) {
            FG_MESSAGE(0),
            BG_MESSAGE(1),
            UPGRADE_MESSAGE(2),
            NO_MESSAGE(3),
            FG_FINE_LOCATION_MESSAGE(4),
            FG_COARSE_LOCATION_MESSAGE(5)
        }
    }
}

/**
 * Factory for an AppPermissionViewModel
 *
 * @param app The current application
 * @param packageName The name of the package this ViewModel represents
 */
class GrantPermissionsViewModelFactory(
    private val app: Application,
    private val packageName: String,
    private val requestedPermissions: Array<String>,
    private val sessionId: Long,
    private val savedState: Bundle?
) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        @Suppress("UNCHECKED_CAST")
        return GrantPermissionsViewModel(app, packageName, requestedPermissions.toList(), sessionId,
            savedState) as T
    }
}
