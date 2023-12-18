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

import com.android.bedstead.harrier.BedsteadJUnit4
import com.android.bedstead.harrier.DeviceState
import com.android.bedstead.harrier.annotations.EnsureHasWorkProfile
import com.android.bedstead.harrier.annotations.Postsubmit
import com.android.bedstead.harrier.annotations.RequireRunOnPrimaryUser
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@EnsureHasWorkProfile(forUser = DeviceState.UserType.PRIMARY_USER)
@RunWith(BedsteadJUnit4::class)
class DomainVerificationWorkProfileAllowParentLinkingTests :
    DomainVerificationWorkProfileTestsBase() {

    private var initialAppLinkPolicy: Boolean? = null

    @Before
    fun saveAndSetPolicy() {
        val manager = deviceState.getWorkDevicePolicyManager()
        initialAppLinkPolicy = manager.getAppLinkPolicy()
        if (initialAppLinkPolicy != true) {
            manager.setAppLinkPolicy(true)
        }
    }

    @After
    fun resetPolicy() {
        val manager = deviceState.getWorkDevicePolicyManager()
        if (initialAppLinkPolicy ?: return != manager.getAppLinkPolicy()) {
            manager.setAppLinkPolicy(initialAppLinkPolicy!!)
        }
    }

    @RequireRunOnPrimaryUser
    @Postsubmit(reason = "New test")
    @Test
    override fun inPersonal_verifiedInOtherProfile() {
        verify(WORK_APP)

        // General configuration does not allow parent -> managed, so only browsers returned
        assertResolvesTo(personalBrowsers)
    }
}
