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

import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.admin.DeviceAdminInfo.PolicyInfo;
import android.util.Log;

import androidx.preference.PreferenceGroup;

import com.android.car.settings.R;
import com.android.car.settings.common.PreferenceController;
import com.android.car.ui.preference.CarUiPreference;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

import java.util.ArrayList;
import java.util.List;

public final class DeviceAdminAddPoliciesPreferenceControllerTest extends
        BaseDeviceAdminAddPreferenceControllerTestCase
                <DeviceAdminAddPoliciesPreferenceController> {

    private static final String TAG = DeviceAdminAddPoliciesPreferenceControllerTest.class
            .getSimpleName();

    private DeviceAdminAddPoliciesPreferenceController mController;

    @Mock
    private PreferenceGroup mPreferenceGroup;

    @Before
    public void setController() {
        mController = new DeviceAdminAddPoliciesPreferenceController(mSpiedContext,
                mPreferenceKey, mFragmentController, mUxRestrictions);
        mController.setDeviceAdmin(mDefaultDeviceAdminInfo);
    }

    @Test
    public void testGetPreferenceType() throws Exception {
        assertWithMessage("preference type").that(mController.getPreferenceType())
                .isEqualTo(PreferenceGroup.class);
    }

    @Test
    public void testGetAvailabilityStatus_noAdmin() throws Exception {
        DeviceAdminAddPoliciesPreferenceController controller =
                new DeviceAdminAddPoliciesPreferenceController(mSpiedContext, mPreferenceKey,
                        mFragmentController, mUxRestrictions);

        assertAvailability(controller.getAvailabilityStatus(),
                PreferenceController.CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void testGetAvailabilityStatus_deviceOwner() throws Exception {
        mockDeviceOwner();

        assertAvailability(mController.getAvailabilityStatus(),
                PreferenceController.DISABLED_FOR_PROFILE);
    }

    @Test
    public void testGetAvailabilityStatus_profileOwner() throws Exception {
        mockProfileOwner();

        assertAvailability(mController.getAvailabilityStatus(),
                PreferenceController.DISABLED_FOR_PROFILE);
    }

    @Test
    public void testGetAvailabilityStatus_regularAdmin() throws Exception {
        // Admin is neither PO nor DO

        assertAvailability(mController.getAvailabilityStatus(), PreferenceController.AVAILABLE);
    }

    @Test
    public void testUpdateState_adminUser() throws Exception {
        updateStateTest(/* isAdmin= */ true);
    }

    @Test
    public void testUpdateState_nonAdminUser() throws Exception {
        updateStateTest(/* isAdmin= */ false);
    }

    private void updateStateTest(boolean isAdmin) {
        // Arrange
        if (isAdmin) {
            mockAdminUser();
        } else {
            mockNonAdminUser();
        }
        ArrayList<PolicyInfo> usedPolicies = mFancyDeviceAdminInfo.getUsedPolicies();
        Log.d(TAG, "Admin policies: " + usedPolicies);
        mController.setDeviceAdmin(mFancyDeviceAdminInfo);
        List<CarUiPreference> addedPreferences = new ArrayList<>();
        when(mPreferenceGroup.addPreference(any())).thenAnswer(call -> {
            Log.d(TAG, "Mocking " + call);
            addedPreferences.add((CarUiPreference) call.getArguments()[0]);
            return true;
        });

        // Act
        mController.updateState(mPreferenceGroup);
        Log.d(TAG, "Added preferences: " + addedPreferences);

        // Assert

        verify(mPreferenceGroup).removeAll();

        int maxPoliciesShown = mRealContext.getResources()
                .getInteger(R.integer.max_device_policies_shown);
        Log.d(TAG, "R.integer.max_device_policies_shown: " + maxPoliciesShown);
        verify(mPreferenceGroup).setInitialExpandedChildrenCount(maxPoliciesShown);

        int expectedSize = usedPolicies.size();
        assertWithMessage("added preferences").that(addedPreferences).hasSize(expectedSize);

        for (int i = 0; i < expectedSize; i++) {
            CarUiPreference preference = addedPreferences.get(i);
            PolicyInfo policy = usedPolicies.get(i);
            CharSequence expectedTitle = mRealContext
                    .getText(isAdmin ? policy.label : policy.labelForSecondaryUsers);
            assertWithMessage("title for policy at index %s", i)
                    .that(preference.getTitle().toString())
                    .isEqualTo(expectedTitle);
            CharSequence expectedSummary = mRealContext
                    .getText(isAdmin ? policy.description : policy.descriptionForSecondaryUsers);
            assertWithMessage("summary for policy at index %s", i)
                    .that(preference.getSummary().toString())
                    .isEqualTo(expectedSummary);
        }
    }
}
