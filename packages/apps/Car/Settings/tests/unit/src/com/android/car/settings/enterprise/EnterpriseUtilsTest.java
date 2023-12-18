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
package com.android.car.settings.enterprise;

import static com.android.car.settings.enterprise.EnterpriseUtils.getAdminWithinPackage;
import static com.android.car.settings.enterprise.EnterpriseUtils.getDeviceAdminInfo;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.admin.DeviceAdminInfo;
import android.content.ComponentName;

import org.junit.Test;

public final class EnterpriseUtilsTest extends BaseEnterpriseTestCase {

    @Test
    public void testGetAdminWithinPackage_notFound() {
        ComponentName admin = getAdminWithinPackage(mSpiedContext, mPackageName);

        assertWithMessage("Admin for %s", mPackageName).that(admin).isNull();
    }

    @Test
    public void testGetAdminWithinPackage_found() {
        mockActiveAdmin(mDefaultAdmin);

        ComponentName admin = getAdminWithinPackage(mSpiedContext, mPackageName);

        assertWithMessage("Admin for %s", mPackageName).that(admin).isEqualTo(mDefaultAdmin);
    }

    @Test
    public void testGetDeviceAdminInfo_notFound() {
        ComponentName admin = new ComponentName("Bond", "James Bond");

        DeviceAdminInfo info = getDeviceAdminInfo(mSpiedContext, admin);

        assertWithMessage("Device admin for %s", admin).that(info).isNull();
    }

    @Test
    public void testGetDeviceAdminInfo_found() {
        DeviceAdminInfo info = getDeviceAdminInfo(mSpiedContext, mDefaultAdmin);

        assertWithMessage("Device admin for %s", mDefaultAdmin).that(info).isNotNull();
        assertWithMessage("Component for %s", info).that(info.getComponent())
                .isEqualTo(mDefaultAdmin);
    }
}
