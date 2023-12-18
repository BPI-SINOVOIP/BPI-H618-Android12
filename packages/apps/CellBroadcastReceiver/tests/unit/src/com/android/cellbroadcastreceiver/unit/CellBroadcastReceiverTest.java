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

package com.android.cellbroadcastreceiver.unit;

import static android.telephony.SubscriptionManager.INVALID_SUBSCRIPTION_ID;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.content.ContentResolver;
import android.content.Context;
import android.content.IContentProvider;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.media.AudioDeviceInfo;
import android.os.RemoteException;
import android.os.UserManager;
import android.provider.Telephony;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.cdma.CdmaSmsCbProgramData;

import com.android.cellbroadcastreceiver.CellBroadcastAlertService;
import com.android.cellbroadcastreceiver.CellBroadcastListActivity;
import com.android.cellbroadcastreceiver.CellBroadcastReceiver;
import com.android.cellbroadcastreceiver.CellBroadcastSettings;
import com.android.cellbroadcastreceiver.R;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

import java.util.ArrayList;
import java.util.List;

public class CellBroadcastReceiverTest extends CellBroadcastTest {
    private static final long MAX_INIT_WAIT_MS = 5000;

    CellBroadcastReceiver mCellBroadcastReceiver;
    String mPackageName = "testPackageName";

    @Mock
    UserManager mUserManager;
    @Mock
    Intent mIntent;
    @Mock
    PackageManager mPackageManager;
    @Mock
    PackageInfo mPackageInfo;
    @Mock
    ContentResolver mContentResolver;
    @Mock
    IContentProvider mContentProviderClient;
    @Mock
    TelephonyManager mMockTelephonyManager;
    @Mock
    SubscriptionManager mSubscriptionManager;
    FakeSharedPreferences mFakeSharedPreferences = new FakeSharedPreferences();

    private Configuration mConfiguration = new Configuration();
    private AudioDeviceInfo[] mDevices = new AudioDeviceInfo[0];
    private Object mLock = new Object();
    private boolean mReady;

    protected void waitUntilReady() {
        synchronized (mLock) {
            if (!mReady) {
                try {
                    mLock.wait(MAX_INIT_WAIT_MS);
                } catch (InterruptedException ie) {
                }

                if (!mReady) {
                    Assert.fail("Telephony tests failed to initialize");
                }
            }
        }
    }

    protected void setReady(boolean ready) {
        synchronized (mLock) {
            mReady = ready;
            mLock.notifyAll();
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(this.getClass().getSimpleName());
        doReturn(mConfiguration).when(mResources).getConfiguration();
        mCellBroadcastReceiver = spy(new CellBroadcastReceiver());
        doReturn(mResources).when(mCellBroadcastReceiver).getResourcesMethod();
        doNothing().when(mCellBroadcastReceiver).startConfigServiceToEnableChannels();
        doReturn(mContext).when(mContext).getApplicationContext();
        doReturn(mPackageName).when(mContext).getPackageName();
        doReturn(mFakeSharedPreferences).when(mContext).getSharedPreferences(anyString(), anyInt());
        doReturn(mUserManager).when(mContext).getSystemService(Context.USER_SERVICE);
        doReturn(false).when(mUserManager).isSystemUser();
        setContext();
    }

    @Test
    public void testOnReceive_actionCarrierConfigChanged() {
        doReturn(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED).when(mIntent).getAction();
        doNothing().when(mCellBroadcastReceiver).enableLauncher();
        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mCellBroadcastReceiver).initializeSharedPreference(any(), anyInt());
        verify(mCellBroadcastReceiver).startConfigServiceToEnableChannels();
        verify(mCellBroadcastReceiver).enableLauncher();
    }

    @Test
    public void testOnReceive_actionCarrierConfigChangedOnRebroadcast() {
        doReturn(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED).when(mIntent).getAction();
        doReturn(true).when(mIntent)
                .getBooleanExtra("android.telephony.extra.REBROADCAST_ON_UNLOCK", false);
        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mCellBroadcastReceiver, never()).initializeSharedPreference(any(), anyInt());
        verify(mCellBroadcastReceiver, never()).startConfigServiceToEnableChannels();
        verify(mCellBroadcastReceiver, never()).enableLauncher();
    }

    @Test
    public void testOnReceive_actionBootCompleted() {
        doReturn(mContentResolver).when(mContext).getContentResolver();
        doReturn(mContentProviderClient).when(mContentResolver).acquireContentProviderClient(
                "cellbroadcasts-app");
        doReturn(Intent.ACTION_BOOT_COMPLETED).when(mIntent).getAction();
        mCellBroadcastReceiver.onReceive(mContext, mIntent);
    }

    @Test
    public void testOnReceive_cellbroadcastStartConfigAction() {
        doReturn(CellBroadcastReceiver.CELLBROADCAST_START_CONFIG_ACTION).when(mIntent).getAction();
        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mCellBroadcastReceiver, never()).initializeSharedPreference(any(), anyInt());
        verify(mCellBroadcastReceiver).startConfigServiceToEnableChannels();
    }

    @Test
    public void testOnReceive_actionDefaultSmsSubscriptionChanged() {
        doReturn(SubscriptionManager.ACTION_DEFAULT_SMS_SUBSCRIPTION_CHANGED)
                .when(mIntent).getAction();
        doReturn(mUserManager).when(mContext).getSystemService(anyString());
        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mCellBroadcastReceiver, never()).initializeSharedPreference(any(), anyInt());
        verify(mCellBroadcastReceiver).startConfigServiceToEnableChannels();
    }

    @Test
    public void testOnReceive_actionSmsEmergencyCbReceived() {
        doReturn(Telephony.Sms.Intents.ACTION_SMS_EMERGENCY_CB_RECEIVED).when(mIntent).getAction();
        doReturn(mIntent).when(mIntent).setClass(mContext, CellBroadcastAlertService.class);
        doReturn(null).when(mContext).startService(mIntent);

        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mIntent).setClass(mContext, CellBroadcastAlertService.class);
        verify(mContext).startService(mIntent);
    }

    @Test
    public void testOnReceive_smsCbReceivedAction() {
        doReturn(Telephony.Sms.Intents.SMS_CB_RECEIVED_ACTION).when(mIntent).getAction();
        doReturn(mIntent).when(mIntent).setClass(mContext, CellBroadcastAlertService.class);
        doReturn(null).when(mContext).startService(any());

        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mIntent).setClass(mContext, CellBroadcastAlertService.class);
        verify(mContext).startService(mIntent);
    }

    @Test
    public void testOnReceive_smsServiceCategoryProgramDataReceivedAction() {
        doReturn(Telephony.Sms.Intents.SMS_SERVICE_CATEGORY_PROGRAM_DATA_RECEIVED_ACTION)
                .when(mIntent).getAction();
        doReturn(null).when(mIntent).getParcelableArrayListExtra(anyString());

        mCellBroadcastReceiver.onReceive(mContext, mIntent);
        verify(mIntent).getParcelableArrayListExtra(anyString());
    }

    @Test
    public void testInitializeSharedPreference_ifSystemUser_invalidSub() {
        doReturn("An invalid action").when(mIntent).getAction();
        doReturn(true).when(mUserManager).isSystemUser();
        doReturn(true).when(mCellBroadcastReceiver).sharedPrefsHaveDefaultValues();
        doNothing().when(mCellBroadcastReceiver).adjustReminderInterval();

        mCellBroadcastReceiver.initializeSharedPreference(mContext, INVALID_SUBSCRIPTION_ID);
        verify(mContext, never()).startService(any());
        // Check interval.
    }

    private void mockTelephonyManager() {
        doReturn(mMockTelephonyManager).when(mMockTelephonyManager)
                .createForSubscriptionId(anyInt());
        doReturn(Context.TELEPHONY_SERVICE).when(mContext).getSystemServiceName(
                TelephonyManager.class);
        doReturn(mMockTelephonyManager).when(mContext).getSystemService(Context.TELEPHONY_SERVICE);
    }

    @Test
    public void testInitializeSharedPreference_ifSystemUser_firstSub() throws Exception {
        doReturn("An invalid action").when(mIntent).getAction();
        doReturn(true).when(mUserManager).isSystemUser();
        doReturn(true).when(mCellBroadcastReceiver).sharedPrefsHaveDefaultValues();
        doNothing().when(mCellBroadcastReceiver).adjustReminderInterval();
        mockTelephonyManager();

        int subId = 1;
        int otherSubId = 2;
        // The subId has to match default sub for it to take action.
        doReturn(SubscriptionManager.INVALID_SUBSCRIPTION_ID).when(mSubService).getDefaultSubId();
        mCellBroadcastReceiver.initializeSharedPreference(mContext, subId);
        verify(mContext, never()).startService(any());

        // Not starting ConfigService, not matching default subId.
        doReturn(otherSubId).when(mSubService).getDefaultSubId();
        mCellBroadcastReceiver.initializeSharedPreference(mContext, subId);
        verify(mContext, never()).startService(any());

        // Not starting ConfigService, simCarrierId is UNKNOWN.
        doReturn(subId).when(mSubService).getDefaultSubId();
        doReturn(TelephonyManager.UNKNOWN_CARRIER_ID).when(mMockTelephonyManager)
                .getSimCarrierId();
        mCellBroadcastReceiver.initializeSharedPreference(mContext, subId);
        verify(mContext, never()).startService(any());

        // Not starting ConfigService, as there was no previous carrierId.
        doReturn(subId).when(mMockTelephonyManager).getSimCarrierId();
        mCellBroadcastReceiver.initializeSharedPreference(mContext, subId);
        verify(mContext, never()).startService(any());
    }

    @Test
    public void testInitializeSharedPreference_ifSystemUser_carrierChange() throws Exception {
        doReturn("An invalid action").when(mIntent).getAction();
        doReturn(true).when(mUserManager).isSystemUser();
        doReturn(true).when(mCellBroadcastReceiver).sharedPrefsHaveDefaultValues();
        doNothing().when(mCellBroadcastReceiver).adjustReminderInterval();
        mockTelephonyManager();

        int firstSubId = 1;
        int secondSubId = 2;
        // Initialize for first sub.
        doReturn(firstSubId).when(mSubService).getDefaultSubId();
        doReturn(firstSubId).when(mMockTelephonyManager).getSimCarrierId();
        mCellBroadcastReceiver.initializeSharedPreference(mContext, firstSubId);
        verify(mContext, never()).startService(any());

        // InitializeSharedPreference for second sub.
        // Starting ConfigService, as there's a carrierId change.
        doReturn(secondSubId).when(mSubService).getDefaultSubId();
        doReturn(secondSubId).when(mMockTelephonyManager).getSimCarrierId();
        mCellBroadcastReceiver.initializeSharedPreference(mContext, secondSubId);
        verify(mContext).startService(any());
    }

    @Test
    public void testInitializeSharedPreference_ifNotSystemUser() {
        doReturn("An invalid action").when(mIntent).getAction();
        doReturn(false).when(mUserManager).isSystemUser();

        mCellBroadcastReceiver.initializeSharedPreference(any(), anyInt());
        assertThat(mFakeSharedPreferences.getValueCount()).isEqualTo(0);
    }

    @Test
    public void testMigrateSharedPreferenceFromLegacyWhenNoLegacyProvider() {
        doReturn(mContentResolver).when(mContext).getContentResolver();
        doReturn(null).when(mContentResolver).acquireContentProviderClient(
                Telephony.CellBroadcasts.AUTHORITY_LEGACY);

        mCellBroadcastReceiver.migrateSharedPreferenceFromLegacy();
        verify(mContext, never()).getSharedPreferences(anyString(), anyInt());
    }

    @Test
    public void testMigrateSharedPreferenceFromLegacyWhenBundleNull() throws RemoteException {
        doReturn(mContentResolver).when(mContext).getContentResolver();
        doReturn(mContentProviderClient).when(mContentResolver).acquireContentProviderClient(
                Telephony.CellBroadcasts.AUTHORITY_LEGACY);
        doReturn(null).when(mContentProviderClient).call(
                anyString(), anyString(), anyString(), any());

        mCellBroadcastReceiver.migrateSharedPreferenceFromLegacy();
        verify(mContext).getSharedPreferences(anyString(), anyInt());
        assertThat(mFakeSharedPreferences.getValueCount()).isEqualTo(0);
    }

    @Test
    public void testSetTestingMode() {
        assertThat(mCellBroadcastReceiver.isTestingMode(mContext)).isFalse();
        mCellBroadcastReceiver.setTestingMode(true);
        assertThat(mCellBroadcastReceiver.isTestingMode(mContext)).isTrue();
    }

    @Test
    public void testAdjustReminderInterval() {
        mFakeSharedPreferences.putString(CellBroadcastReceiver.CURRENT_INTERVAL_DEFAULT,
                "currentInterval");
        doReturn(mResources).when(mContext).getResources();
        doReturn(mContext).when(mContext).createConfigurationContext(any());
        doReturn(mSubscriptionManager).when(mContext).getSystemService(
                Context.TELEPHONY_SUBSCRIPTION_SERVICE);
        doReturn("newInterval").when(mResources).getString(
                R.string.alert_reminder_interval_in_min_default);

        mCellBroadcastReceiver.adjustReminderInterval();
        assertThat(mFakeSharedPreferences.getString(
                CellBroadcastReceiver.CURRENT_INTERVAL_DEFAULT, ""))
                .isEqualTo("newInterval");
    }

    @Test
    public void testEnableLauncherIfNoLauncherActivity() throws
            PackageManager.NameNotFoundException {
        doReturn(mPackageManager).when(mContext).getPackageManager();
        doReturn(mPackageInfo).when(mPackageManager).getPackageInfo(anyString(), anyInt());

        ActivityInfo activityInfo = new ActivityInfo();
        String activityInfoName = "";
        activityInfo.targetActivity = CellBroadcastListActivity.class.getName();
        activityInfo.name = activityInfoName;
        ActivityInfo[] activityInfos = new ActivityInfo[1];
        activityInfos[0] = activityInfo;
        mPackageInfo.activities = activityInfos;

        mCellBroadcastReceiver.enableLauncher();
        verify(mPackageManager, never()).setComponentEnabledSetting(any(), anyInt(), anyInt());
    }

    @Test
    public void testEnableLauncherIfEnableTrue() throws PackageManager.NameNotFoundException {
        doReturn(mPackageManager).when(mContext).getPackageManager();
        doReturn(mPackageInfo).when(mPackageManager).getPackageInfo(anyString(), anyInt());
        doReturn(true).when(mResources)
                .getBoolean(R.bool.show_message_history_in_launcher);

        ActivityInfo activityInfo = new ActivityInfo();
        String activityInfoName = "testName";
        activityInfo.targetActivity = CellBroadcastListActivity.class.getName();
        activityInfo.name = activityInfoName;
        ActivityInfo[] activityInfos = new ActivityInfo[1];
        activityInfos[0] = activityInfo;
        mPackageInfo.activities = activityInfos;

        mCellBroadcastReceiver.enableLauncher();
        verify(mPackageManager).setComponentEnabledSetting(any(),
                eq(PackageManager.COMPONENT_ENABLED_STATE_ENABLED), anyInt());
    }

    @Test
    public void testEnableLauncherIfEnableFalse() throws PackageManager.NameNotFoundException {
        doReturn(mPackageManager).when(mContext).getPackageManager();
        doReturn(mPackageInfo).when(mPackageManager).getPackageInfo(anyString(), anyInt());
        doReturn(false).when(mResources)
                .getBoolean(R.bool.show_message_history_in_launcher);

        ActivityInfo activityInfo = new ActivityInfo();
        String activityInfoName = "testName";
        activityInfo.targetActivity = CellBroadcastListActivity.class.getName();
        activityInfo.name = activityInfoName;
        ActivityInfo[] activityInfos = new ActivityInfo[1];
        activityInfos[0] = activityInfo;
        mPackageInfo.activities = activityInfos;

        mCellBroadcastReceiver.enableLauncher();
        verify(mPackageManager).setComponentEnabledSetting(any(),
                eq(PackageManager.COMPONENT_ENABLED_STATE_DISABLED), anyInt());
    }

    @Test
    public void testTryCdmaSetCatergory() {
        boolean enable = true;

        mCellBroadcastReceiver.tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_EXTREME_THREAT, enable);
        assertThat(mFakeSharedPreferences.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS, !enable))
                .isEqualTo(enable);

        mCellBroadcastReceiver.tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_SEVERE_THREAT, enable);
        assertThat(mFakeSharedPreferences.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS, !enable))
                .isEqualTo(enable);

        mCellBroadcastReceiver.tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_CHILD_ABDUCTION_EMERGENCY, enable);
        assertThat(mFakeSharedPreferences.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_CMAS_AMBER_ALERTS, !enable))
                .isEqualTo(enable);

        mCellBroadcastReceiver.tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_TEST_MESSAGE, enable);
        assertThat(mFakeSharedPreferences.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_TEST_ALERTS, !enable))
                .isEqualTo(enable);
    }

    @Test
    public void testHandleCdmaSmsCbProgramDataOperationAddAndDelete() {
        CdmaSmsCbProgramData programData = new CdmaSmsCbProgramData(
                CdmaSmsCbProgramData.OPERATION_ADD_CATEGORY,
                CdmaSmsCbProgramData.CATEGORY_CMAS_EXTREME_THREAT,
                1, 1, 1, "catergoryName");
        mCellBroadcastReceiver.handleCdmaSmsCbProgramData(new ArrayList<>(List.of(programData)));
        verify(mCellBroadcastReceiver).tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_EXTREME_THREAT, true);

        programData = new CdmaSmsCbProgramData(CdmaSmsCbProgramData.OPERATION_DELETE_CATEGORY,
                CdmaSmsCbProgramData.CATEGORY_CMAS_EXTREME_THREAT,
                1, 1, 1, "catergoryName");
        mCellBroadcastReceiver.handleCdmaSmsCbProgramData(new ArrayList<>(List.of(programData)));
        verify(mCellBroadcastReceiver).tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_EXTREME_THREAT, false);
    }

    @Test
    public void testHandleCdmaSmsCbProgramDataOprationClear() {
        CdmaSmsCbProgramData programData = new CdmaSmsCbProgramData(
                CdmaSmsCbProgramData.OPERATION_CLEAR_CATEGORIES,
                CdmaSmsCbProgramData.CATEGORY_CMAS_PRESIDENTIAL_LEVEL_ALERT,
                1, 1, 1, "catergoryName");
        mCellBroadcastReceiver.handleCdmaSmsCbProgramData(new ArrayList<>(List.of(programData)));
        verify(mCellBroadcastReceiver).tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_EXTREME_THREAT, false);
        verify(mCellBroadcastReceiver).tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_SEVERE_THREAT, false);
        verify(mCellBroadcastReceiver).tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_CHILD_ABDUCTION_EMERGENCY, false);
        verify(mCellBroadcastReceiver).tryCdmaSetCategory(mContext,
                CdmaSmsCbProgramData.CATEGORY_CMAS_TEST_MESSAGE, false);
    }

    //this method is just to assign mContext to the spied instance mCellBroadcastReceiver
    private void setContext() {
        doReturn("dummy action").when(mIntent).getAction();

        mCellBroadcastReceiver.onReceive(mContext, mIntent);
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }
}
