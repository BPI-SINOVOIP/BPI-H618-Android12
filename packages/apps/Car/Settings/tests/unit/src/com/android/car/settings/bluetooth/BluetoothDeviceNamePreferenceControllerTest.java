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

package com.android.car.settings.bluetooth;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assume.assumeTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothProfile;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.pm.PackageManager;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.Preference;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.BluetoothTestUtils;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.settingslib.bluetooth.BluetoothUtils;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.bluetooth.CachedBluetoothDeviceManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.StringJoiner;

@RunWith(AndroidJUnit4.class)
public class BluetoothDeviceNamePreferenceControllerTest {
    private LifecycleOwner mLifecycleOwner;
    private Context mContext = ApplicationProvider.getApplicationContext();
    private BluetoothDeviceNamePreferenceController mPreferenceController;
    private Preference mPreference;
    private CarUxRestrictions mCarUxRestrictions;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private CachedBluetoothDevice mCachedDevice;
    @Mock
    private CachedBluetoothDeviceManager mCachedDeviceManager;

    @Before
    public void setUp() {
        mLifecycleOwner = new TestLifecycleOwner();
        MockitoAnnotations.initMocks(this);

        // Ensure bluetooth is available and enabled.
        assumeTrue(mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH));
        BluetoothTestUtils.setBluetoothState(mContext, /* enable= */ true);

        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        mPreference = new Preference(mContext);
        mPreference.setSelectable(true);
        mPreferenceController = new TestBluetoothDeviceNamePreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        mPreferenceController.setCachedDevice(mCachedDevice);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreference);
        mPreferenceController.onCreate(mLifecycleOwner);
    }

    @Test
    public void refreshUi_setsDeviceNameAsTitle() {
        String name = "name";
        when(mCachedDevice.getName()).thenReturn(name);

        mPreferenceController.refreshUi();

        assertThat(mPreference.getTitle()).isEqualTo(name);
    }

    @Test
    public void refreshUi_connected_setsCarConnectionSummaryAsSummary() {
        String summary = "summary";
        when(mCachedDevice.isConnected()).thenReturn(true);
        when(mCachedDevice.getCarConnectionSummary()).thenReturn(summary);

        mPreferenceController.refreshUi();

        assertThat(mPreference.getSummary()).isEqualTo(summary);
    }

    @Test
    public void refreshUi_notConnected_setsDisconnectedAsSummary() {
        when(mCachedDevice.isConnected()).thenReturn(false);

        mPreferenceController.refreshUi();

        assertThat(mPreference.getSummary()).isEqualTo(mContext.getString(BluetoothUtils
                .getConnectionStateSummary(BluetoothProfile.STATE_DISCONNECTED)));
    }

    @Test
    public void refreshUi_setsIcon() {
        when(mCachedDevice.getBtClass()).thenReturn(
                new BluetoothClass(BluetoothClass.Device.Major.PHONE));

        mPreferenceController.refreshUi();

        assertThat(mPreference.getIcon()).isNotNull();
    }

    @Test
    public void refreshUi_hearingAidDevice_setsBatteryStatusesAsSummary() {
        String summary = "summary";
        when(mCachedDevice.isConnected()).thenReturn(true);
        when(mCachedDevice.getCarConnectionSummary()).thenReturn(summary);
        String otherSummary = "other summary";
        when(mCachedDeviceManager.getSubDeviceSummary(mCachedDevice)).thenReturn("other summary");

        mPreferenceController.refreshUi();

        String expected = new StringJoiner(System.lineSeparator()).add(summary).add(
                otherSummary).toString();
        assertThat(mPreference.getSummary()).isEqualTo(expected);
    }

    @Test
    public void preferenceClicked_launchesRenameDialog() {
        mPreferenceController.onStart(mLifecycleOwner);

        mPreference.performClick();

        verify(mFragmentController).showDialog(any(RemoteRenameDialogFragment.class),
                eq(RemoteRenameDialogFragment.TAG));
    }

    @Test
    public void preferenceClicked_handled() {
        mPreferenceController.onStart(mLifecycleOwner);

        assertThat(mPreference.getOnPreferenceClickListener()
                .onPreferenceClick(mPreference)).isTrue();
    }

    private class TestBluetoothDeviceNamePreferenceController
            extends BluetoothDeviceNamePreferenceController {

        TestBluetoothDeviceNamePreferenceController(Context context, String preferenceKey,
                FragmentController fragmentController,
                CarUxRestrictions uxRestrictions) {
            super(context, preferenceKey, fragmentController, uxRestrictions);
        }

        @Override
        CachedBluetoothDeviceManager getCachedDeviceManager() {
            return mCachedDeviceManager;
        }
    }
}
