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

package com.android.permissioncontroller.permission.ui

import android.Manifest.permission.ACCESS_COARSE_LOCATION
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.runner.RunWith

/**
 * Simple tests for {@link PermissionAppsFragment} when showing location permission
 *
 * Currently, does NOT run on TV (same as the other tests that extend [PermissionAppsFragmentTest]).
 * TODO(b/178576541): Adapt and run on TV.
 */
@RunWith(AndroidJUnit4::class)
class LocationPermissionAppsFragmentTest : PermissionAppsFragmentTest(
    "/data/local/tmp/permissioncontroller/tests/inprocess/AppThatRequestsLocation.apk",
    "android.permission.cts.appthatrequestpermission",
    ACCESS_COARSE_LOCATION
)
