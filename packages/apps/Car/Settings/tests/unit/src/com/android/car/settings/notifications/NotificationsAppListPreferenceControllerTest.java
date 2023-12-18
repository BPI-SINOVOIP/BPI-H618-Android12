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

package com.android.car.settings.notifications;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.INotificationManager;
import android.app.NotificationChannel;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.pm.ApplicationInfo;

import androidx.lifecycle.LifecycleOwner;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceGroup;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;
import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.android.car.settings.R;
import com.android.car.settings.applications.ApplicationDetailsFragment;
import com.android.car.settings.common.FragmentController;
import com.android.car.settings.common.PreferenceControllerTestUtil;
import com.android.car.settings.testutils.TestLifecycleOwner;
import com.android.car.ui.preference.CarUiTwoActionSwitchPreference;
import com.android.settingslib.applications.ApplicationsState;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;

@RunWith(AndroidJUnit4.class)
public class NotificationsAppListPreferenceControllerTest {
    private static final String PKG_NAME = "package.name";
    private static final int UID = 1001010;
    private static final int ID = 1;
    private static final String LABEL = "label";

    private Context mContext = ApplicationProvider.getApplicationContext();
    private LifecycleOwner mLifecycleOwner;
    private PreferenceGroup mPreferenceCategory;
    private NotificationsAppListPreferenceController mPreferenceController;
    private CarUxRestrictions mCarUxRestrictions;
    private ArrayList<ApplicationsState.AppEntry> mAppEntryList;

    @Mock
    private FragmentController mFragmentController;
    @Mock
    private INotificationManager mMockManager;
    @Mock
    private NotificationChannel mMockChannel;

    @Before
    @UiThreadTest
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mLifecycleOwner = new TestLifecycleOwner();
        mCarUxRestrictions = new CarUxRestrictions.Builder(/* reqOpt= */ true,
                CarUxRestrictions.UX_RESTRICTIONS_BASELINE, /* timestamp= */ 0).build();

        PreferenceManager preferenceManager = new PreferenceManager(mContext);
        PreferenceScreen screen = preferenceManager.createPreferenceScreen(mContext);
        mPreferenceCategory = new PreferenceCategory(mContext);
        screen.addPreference(mPreferenceCategory);

        mPreferenceController = new NotificationsAppListPreferenceController(mContext,
                /* preferenceKey= */ "key", mFragmentController, mCarUxRestrictions);
        PreferenceControllerTestUtil.assignPreference(mPreferenceController, mPreferenceCategory);

        mPreferenceController.mNotificationManager = mMockManager;

        ApplicationInfo applicationInfo = new ApplicationInfo();
        applicationInfo.packageName = PKG_NAME;
        applicationInfo.uid = UID;
        applicationInfo.sourceDir = "";
        ApplicationsState.AppEntry appEntry =
                new ApplicationsState.AppEntry(mContext, applicationInfo, ID);
        appEntry.label = LABEL;
        appEntry.icon = mContext.getDrawable(R.drawable.test_icon);

        mAppEntryList = new ArrayList<>();
        mAppEntryList.add(appEntry);
    }

    @Test
    public void onCreate_createsPreference() throws Exception {
        when(mMockManager.areNotificationsEnabledForPackage(PKG_NAME, UID)).thenReturn(true);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        assertThat(mPreferenceCategory.getPreferenceCount()).isEqualTo(1);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);

        assertThat(preference.getTitle()).isEqualTo(LABEL);
        assertThat(preference.getIcon()).isNotNull();
    }

    @Test
    public void onCreate_notificationEnabled_isChecked() throws Exception {
        when(mMockManager.areNotificationsEnabledForPackage(PKG_NAME, UID)).thenReturn(true);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);

        assertThat(preference.isSecondaryActionChecked()).isTrue();
    }

    @Test
    public void onCreate_notificationDisabled_isNotChecked() throws Exception {
        when(mMockManager.areNotificationsEnabledForPackage(PKG_NAME, UID)).thenReturn(false);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);

        assertThat(preference.isSecondaryActionChecked()).isFalse();
    }

    @Test
    public void toggle_setEnable_enablingNotification() throws Exception {
        when(mMockManager.areNotificationsEnabledForPackage(PKG_NAME, UID)).thenReturn(false);
        when(mMockManager.onlyHasDefaultChannel(PKG_NAME, UID)).thenReturn(false);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);

        preference.performSecondaryActionClick();

        verify(mMockManager).setNotificationsEnabledForPackage(PKG_NAME, UID, true);
    }

    @Test
    public void toggle_setDisable_disablingNotification() throws Exception {
        when(mMockManager.areNotificationsEnabledForPackage(PKG_NAME, UID)).thenReturn(true);
        when(mMockManager.onlyHasDefaultChannel(PKG_NAME, UID)).thenReturn(false);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);

        preference.performSecondaryActionClick();

        verify(mMockManager).setNotificationsEnabledForPackage(PKG_NAME, UID, false);
    }

    @Test
    public void toggle_onlyHasDefaultChannel_updateChannel() throws Exception {
        when(mMockManager.areNotificationsEnabledForPackage(PKG_NAME, UID)).thenReturn(false);
        when(mMockManager.onlyHasDefaultChannel(PKG_NAME, UID)).thenReturn(true);
        when(mMockManager
                .getNotificationChannelForPackage(
                        PKG_NAME, UID, NotificationChannel.DEFAULT_CHANNEL_ID, null, true))
                .thenReturn(mMockChannel);

        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);

        preference.performSecondaryActionClick();

        verify(mMockManager).updateNotificationChannelForPackage(PKG_NAME, UID, mMockChannel);
    }

    @Test
    @UiThreadTest
    public void clickPreference_shouldOpenApplicationDetailsFragment() {
        mPreferenceController.onCreate(mLifecycleOwner);
        mPreferenceController.onDataLoaded(mAppEntryList);

        CarUiTwoActionSwitchPreference preference = (CarUiTwoActionSwitchPreference)
                mPreferenceCategory.getPreference(0);
        preference.performClick();

        verify(mFragmentController).launchFragment(any(ApplicationDetailsFragment.class));
    }
}
