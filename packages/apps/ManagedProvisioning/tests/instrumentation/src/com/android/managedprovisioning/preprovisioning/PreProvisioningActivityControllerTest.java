/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.managedprovisioning.preprovisioning;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.CODE_MANAGED_USERS_NOT_SUPPORTED;
import static android.app.admin.DevicePolicyManager.CODE_OK;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ALLOWED_PROVISIONING_MODES;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_IMEI;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_MODE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SERIAL_NUMBER;
import static android.app.admin.DevicePolicyManager.FLAG_SUPPORTED_MODES_DEVICE_OWNER;
import static android.app.admin.DevicePolicyManager.FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED;
import static android.app.admin.DevicePolicyManager.FLAG_SUPPORTED_MODES_PERSONALLY_OWNED;
import static android.app.admin.DevicePolicyManager.PROVISIONING_MODE_FULLY_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.PROVISIONING_MODE_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.PROVISIONING_MODE_MANAGED_PROFILE_ON_PERSONAL_DEVICE;
import static android.nfc.NfcAdapter.ACTION_NDEF_DISCOVERED;

import static com.android.managedprovisioning.common.Globals.ACTION_RESUME_PROVISIONING;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.testng.Assert.assertThrows;

import android.app.ActivityManager;
import android.app.KeyguardManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.VectorDrawable;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.PersistableBundle;
import android.os.UserHandle;
import android.os.UserManager;
import android.service.persistentdata.PersistentDataBlockManager;
import android.telephony.TelephonyManager;
import android.test.AndroidTestCase;
import android.text.TextUtils;

import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.analytics.TimeLogger;
import com.android.managedprovisioning.common.GetProvisioningModeUtils;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.ManagedProvisioningSharedPreferences;
import com.android.managedprovisioning.common.PolicyComplianceUtils;
import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.PackageDownloadInfo;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.model.WifiInfo;
import com.android.managedprovisioning.parser.MessageParser;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

@SmallTest
public class PreProvisioningActivityControllerTest extends AndroidTestCase {
    private static final String TEST_MDM_PACKAGE = "com.test.mdm";
    private static final String TEST_MDM_PACKAGE_LABEL = "Test MDM";
    private static final ComponentName TEST_MDM_COMPONENT_NAME = new ComponentName(TEST_MDM_PACKAGE,
            "com.test.mdm.DeviceAdmin");
    private static final String TEST_BOGUS_PACKAGE = "com.test.bogus";
    private static final String TEST_WIFI_SSID = "TestNet";
    private static final String MP_PACKAGE_NAME = "com.android.managedprovisioning";
    private static final int TEST_USER_ID = 10;
    private static final PackageDownloadInfo PACKAGE_DOWNLOAD_INFO =
            PackageDownloadInfo.Builder.builder()
                    .setCookieHeader("COOKIE_HEADER")
                    .setLocation("LOCATION")
                    .setMinVersion(1)
                    .setPackageChecksum(new byte[] {1})
                    .setSignatureChecksum(new byte[] {1})
                    .build();
    private static final String TEST_IMEI = "my imei";

    @Mock
    private Context mContext;
    @Mock
    Resources mResources;
    @Mock
    private DevicePolicyManager mDevicePolicyManager;
    @Mock
    private UserManager mUserManager;
    @Mock
    private PackageManager mPackageManager;
    @Mock
    private ActivityManager mActivityManager;
    @Mock
    private KeyguardManager mKeyguardManager;
    @Mock
    private PersistentDataBlockManager mPdbManager;
    @Mock
    private PreProvisioningActivityController.Ui mUi;
    @Mock
    private MessageParser mMessageParser;
    @Mock
    private Utils mUtils;
    @Mock
    private SettingsFacade mSettingsFacade;
    @Mock
    private Intent mIntent;
    @Mock
    private EncryptionController mEncryptionController;
    @Mock
    private TimeLogger mTimeLogger;
    @Mock
    private ManagedProvisioningSharedPreferences mSharedPreferences;
    @Mock
    private TelephonyManager mTelephonyManager;

    private ProvisioningParams mParams;
    private PreProvisioningViewModel mViewModel;

    private PreProvisioningActivityController mController;
    public static final PersistableBundle TEST_ADMIN_BUNDLE = new PersistableBundle();
    static {
        TEST_ADMIN_BUNDLE.putInt("someKey", 123);
    }

    private Handler mHandler = new Handler(Looper.getMainLooper());

    @Override
    public void setUp() throws PackageManager.NameNotFoundException {
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        MockitoAnnotations.initMocks(this);

        when(mContext.getSystemServiceName(DevicePolicyManager.class))
                .thenReturn(Context.DEVICE_POLICY_SERVICE);
        when(mContext.getSystemService(DevicePolicyManager.class))
                .thenReturn(mDevicePolicyManager);

        when(mContext.getSystemServiceName(UserManager.class))
                .thenReturn(Context.USER_SERVICE);
        when(mContext.getSystemService(UserManager.class))
                .thenReturn(mUserManager);

        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getSystemService(Context.ACTIVITY_SERVICE)).thenReturn(mActivityManager);
        when(mContext.getSystemService(Context.KEYGUARD_SERVICE)).thenReturn(mKeyguardManager);
        when(mContext.getSystemService(Context.PERSISTENT_DATA_BLOCK_SERVICE))
                .thenReturn(mPdbManager);
        when(mContext.getPackageName()).thenReturn(MP_PACKAGE_NAME);
        when(mContext.getResources()).thenReturn(
                androidx.test.InstrumentationRegistry.getTargetContext().getResources());
        when(mContext.getSystemServiceName(TelephonyManager.class))
                .thenReturn(Context.TELEPHONY_SERVICE);
        when(mContext.getSystemService(TelephonyManager.class)).thenReturn(mTelephonyManager);
        when(mTelephonyManager.getImei()).thenReturn(TEST_IMEI);

        when(mUserManager.getUserHandle()).thenReturn(TEST_USER_ID);

        when(mUtils.isEncryptionRequired()).thenReturn(false);
        when(mUtils.currentLauncherSupportsManagedProfiles(mContext)).thenReturn(true);
        when(mUserManager.canAddMoreManagedProfiles(anyInt(), anyBoolean())).thenReturn(
                true);

        when(mPackageManager.getApplicationIcon(anyString())).thenReturn(new VectorDrawable());
        when(mPackageManager.getApplicationLabel(any())).thenReturn(TEST_MDM_PACKAGE_LABEL);
        when(mPackageManager.resolveActivity(any(), anyInt())).thenReturn(new ResolveInfo());

        when(mKeyguardManager.inKeyguardRestrictedInputMode()).thenReturn(false);
        when(mDevicePolicyManager.getStorageEncryptionStatus())
                .thenReturn(DevicePolicyManager.ENCRYPTION_STATUS_INACTIVE);
        when(mSettingsFacade.isDuringSetupWizard(mContext)).thenReturn(false);

        mViewModel = new PreProvisioningViewModel(
                mTimeLogger,
                mMessageParser,
                mEncryptionController);

        mController = new PreProvisioningActivityController(
                mContext,
                mUi,
                mUtils,
                mSettingsFacade,
                mSharedPreferences,
                new PolicyComplianceUtils(),
                new GetProvisioningModeUtils(),
                mViewModel);
    }

    public void testManagedProfile() throws Exception {
        // GIVEN an intent to provision a managed profile
        prepareMocksForManagedProfileIntent(false);
        // WHEN initiating provisioning
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE));
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user consents
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.continueProvisioningAfterUserConsent());
        // THEN start profile provisioning
        verify(mUi).startProvisioning(mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_provisioningNotAllowed() throws Exception {
        // GIVEN an intent to provision a managed profile, but provisioning mode is not allowed
        prepareMocksForManagedProfileIntent(false);
        when(mDevicePolicyManager.checkProvisioningPreCondition(
                ACTION_PROVISION_MANAGED_PROFILE, TEST_MDM_PACKAGE))
                .thenReturn(CODE_MANAGED_USERS_NOT_SUPPORTED);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.cant_add_work_profile),
                eq(R.string.work_profile_cant_be_added_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_nullCallingPackage() throws Exception {
        // GIVEN a device that is not currently encrypted
        prepareMocksForManagedProfileIntent(false);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, null);
        // THEN error is shown
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.contact_your_admin_for_help), any(String.class));
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_invalidCallingPackage() throws Exception {
        // GIVEN a device that is not currently encrypted
        prepareMocksForManagedProfileIntent(false);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, "com.android.invalid.dpc");
        // THEN error is shown
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.contact_your_admin_for_help), any(String.class));
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_withEncryption() throws Exception {
        // GIVEN a device that is not currently encrypted
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating managed profile provisioning
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE));
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // THEN show encryption screen
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_afterEncryption() throws Exception {
        // GIVEN managed profile provisioning continues after successful encryption. In this case
        // we don't set the startedByTrustedSource flag.
        prepareMocksForAfterEncryption(ACTION_PROVISION_MANAGED_PROFILE, false);
        // WHEN initiating with a continuation intent
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, MP_PACKAGE_NAME));
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user consents
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.continueProvisioningAfterUserConsent());
        // THEN start profile provisioning
        verify(mUi).startProvisioning(mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_badLauncher() throws Exception {
        // GIVEN that the current launcher does not support managed profiles
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.currentLauncherSupportsManagedProfiles(mContext)).thenReturn(false);
        // WHEN initiating managed profile provisioning
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE));
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN show a dialog indicating that the current launcher is invalid
        verify(mUi).showCurrentLauncherInvalid();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_wrongPackage() throws Exception {
        // GIVEN that the provisioning intent tries to set a package different from the caller
        // as owner of the profile
        prepareMocksForManagedProfileIntent(false);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_BOGUS_PACKAGE);
        // THEN show an error dialog and do not continue
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.contact_your_admin_for_help), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_frp() throws Exception {
        // GIVEN managed profile provisioning is invoked from SUW with FRP active
        prepareMocksForManagedProfileIntent(false);
        when(mSettingsFacade.isDeviceProvisioned(mContext)).thenReturn(false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating managed profile provisioning
        mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        // THEN show an error dialog and do not continue
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.device_has_reset_protection_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testCheckFactoryResetProtection_skipFrp() throws Exception {
        // GIVEN managed profile provisioning is invoked from SUW with FRP active
        when(mSettingsFacade.isDeviceProvisioned(mContext)).thenReturn(false);
        // setting the data block size to any number greater than 0 to simulate FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // GIVEN there is a persistent data package.
        when(mContext.getResources()).thenReturn(mResources);
        when(mResources.getString(anyInt())).thenReturn("test.persistent.data");
        // GIVEN the persistent data package is a system app.
        PackageInfo packageInfo = new PackageInfo();
        ApplicationInfo applicationInfo = new ApplicationInfo();
        applicationInfo.flags = ApplicationInfo.FLAG_SYSTEM;
        packageInfo.applicationInfo = applicationInfo;
        when(mPackageManager.getPackageInfo(eq("test.persistent.data"), anyInt()))
                .thenReturn(packageInfo);

        // WHEN factory reset protection is checked for trusted source device provisioning.
        ProvisioningParams provisioningParams = createParams(true, false, null,
                ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE, TEST_MDM_PACKAGE);
        boolean result = mController.checkFactoryResetProtection(
                provisioningParams, "test.persistent.data");

        // THEN the check is successful despite the FRP data presence.
        assertThat(result).isTrue();
    }

    public void testManagedProfile_skipEncryption() throws Exception {
        // GIVEN an intent to provision a managed profile with skip encryption
        prepareMocksForManagedProfileIntent(true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating provisioning
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE));
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // WHEN the user consents
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.continueProvisioningAfterUserConsent());
        // THEN start profile provisioning
        verify(mUi).startProvisioning(mParams);
        verify(mUi, never()).requestEncryption(any(ProvisioningParams.class));
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testManagedProfile_encryptionNotSupported() throws Exception {
        // GIVEN an intent to provision a managed profile on an unencrypted device that does not
        // support encryption
        prepareMocksForManagedProfileIntent(false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        when(mDevicePolicyManager.getStorageEncryptionStatus())
                .thenReturn(DevicePolicyManager.ENCRYPTION_STATUS_UNSUPPORTED);
        // WHEN initiating provisioning
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE));
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN the UI elements should be updated accordingly
        verifyInitiateProfileOwnerUi();
        // THEN show an error indicating that this device does not support encryption
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.device_doesnt_allow_encryption_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testInitiateProvisioning_withNfc_showsOwnershipDisclaimer() throws Exception {
        // GIVEN provisioning was started via an NFC tap and should show ownership disclaimer
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.shouldShowOwnershipDisclaimerScreen(eq(mParams))).thenReturn(true);

        // WHEN initiating NFC provisioning
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, /* callingPackage= */ null));

        // THEN show the ownership disclaimer
        verify(mUi).showOwnershipDisclaimerScreen(eq(mParams));
        verifyNoMoreInteractions(mUi);
    }

    public void testInitiateProvisioning_withNfc_skipsOwnershipDisclaimer() throws Exception {
        // GIVEN provisioning was started via an NFC tap and should show ownership disclaimer
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.shouldShowOwnershipDisclaimerScreen(eq(mParams))).thenReturn(false);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            // WHEN initiating NFC provisioning
            mController.initiateProvisioning(mIntent, /* callingPackage= */ null);
        });

        // THEN show the ownership disclaimer
        verify(mUi).initiateUi(any());
        verifyNoMoreInteractions(mUi);
    }

    // TODO(b/177575786): Migrate outdated PreProvisioningControllerTest tests to robolectric
    /*
    public void testNfc() throws Exception {
        // GIVEN provisioning was started via an NFC tap and device is already encrypted
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verifyInitiateDeviceOwnerUi();
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_skipEncryption() throws Exception {
        // GIVEN provisioning was started via an NFC tap with encryption skipped
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating NFC provisioning

        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verifyInitiateDeviceOwnerUi();
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verify(mUi, never()).requestEncryption(any(ProvisioningParams.class));
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_withEncryption() throws Exception {
        // GIVEN provisioning was started via an NFC tap with encryption necessary
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN show encryption screen
        verifyInitiateDeviceOwnerUi();
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_afterEncryption() throws Exception {
        // GIVEN provisioning was started via an NFC tap and we have gone through encryption
        // in this case the device gets resumed with the DO intent and startedByTrustedSource flag
        // set
        prepareMocksForAfterEncryption(ACTION_PROVISION_MANAGED_DEVICE, true);
        // WHEN continuing NFC provisioning after encryption
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verifyInitiateDeviceOwnerUi();
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_frp() throws Exception {
        // GIVEN provisioning was started via an NFC tap, but the device is locked with FRP
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.device_has_reset_protection_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testNfc_encryptionNotSupported() throws Exception {
        // GIVEN provisioning was started via an NFC tap, the device is not encrypted and encryption
        // is not supported on the device
        prepareMocksForNfcIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        when(mDevicePolicyManager.getStorageEncryptionStatus())
                .thenReturn(DevicePolicyManager.ENCRYPTION_STATUS_UNSUPPORTED);
        // WHEN initiating NFC provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN show an error dialog
        verifyInitiateDeviceOwnerUi();
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.device_doesnt_allow_encryption_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testQr() throws Exception {
        // GIVEN provisioning was started via a QR code and device is already encrypted
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verifyInitiateDeviceOwnerUi();
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testQr_skipEncryption() throws Exception {
        // GIVEN provisioning was started via a QR code with encryption skipped
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verifyInitiateDeviceOwnerUi();
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verify(mUi, never()).requestEncryption(any());
        verifyNoMoreInteractions(mUi);
    }

    public void testQr_withEncryption() throws Exception {
        // GIVEN provisioning was started via a QR code with encryption necessary
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN show encryption screen
        verifyInitiateDeviceOwnerUi();
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testQr_frp() throws Exception {
        // GIVEN provisioning was started via a QR code, but the device is locked with FRP
        prepareMocksForQrIntent(ACTION_PROVISION_MANAGED_DEVICE, false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating QR provisioning
        mController.initiateProvisioning(mIntent, null, null);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.device_has_reset_protection_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner() throws Exception {
        // GIVEN device owner provisioning was started and device is already encrypted
        prepareMocksForDoIntent(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, null, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateDeviceOwnerUi();
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner_skipEncryption() throws Exception {
        // GIVEN device owner provisioning was started with skip encryption flag
        prepareMocksForDoIntent(true);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, null, TEST_MDM_PACKAGE);
        // THEN the UI elements should be updated accordingly
        verifyInitiateDeviceOwnerUi();
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verify(mUi, never()).requestEncryption(any());
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    // TODO: There is a difference in behaviour here between the managed profile and the device
    // owner case: In managed profile case, we invoke encryption after user clicks next, but in
    // device owner mode we invoke it straight away. Also in theory no need to update
    // the UI elements if we're moving away from this activity straight away.
    public void testDeviceOwner_withEncryption() throws Exception {
        // GIVEN device owner provisioning is started with encryption needed
        prepareMocksForDoIntent(false);
        when(mUtils.isEncryptionRequired()).thenReturn(true);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, null, TEST_MDM_PACKAGE);
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN update the UI elements and show encryption screen
        verifyInitiateDeviceOwnerUi();
        verify(mUi).requestEncryption(mParams);
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner_afterEncryption() throws Exception {
        // GIVEN device owner provisioning is continued after encryption. In this case we do not set
        // the startedByTrustedSource flag.
        prepareMocksForAfterEncryption(ACTION_PROVISION_MANAGED_DEVICE, false);
        // WHEN provisioning is continued
        mController.initiateProvisioning(mIntent, null, null);
        // THEN the UI elements should be updated accordingly
        verifyInitiateDeviceOwnerUi();
        // WHEN the user consents
        mController.continueProvisioningAfterUserConsent();
        // THEN start device owner provisioning
        verify(mUi).startProvisioning(mUserManager.getUserHandle(), mParams);
        verify(mEncryptionController).cancelEncryptionReminder();
        verifyNoMoreInteractions(mUi);
    }

    public void testDeviceOwner_frp() throws Exception {
        // GIVEN device owner provisioning is invoked with FRP active
        prepareMocksForDoIntent(false);
        // setting the data block size to any number greater than 0 should invoke FRP.
        when(mPdbManager.getDataBlockSize()).thenReturn(4);
        // WHEN initiating provisioning
        mController.initiateProvisioning(mIntent, null, TEST_MDM_PACKAGE);
        // THEN show an error dialog
        verify(mUi).showErrorAndClose(eq(R.string.cant_set_up_device),
                eq(R.string.device_has_reset_protection_contact_admin), any());
        verifyNoMoreInteractions(mUi);
    }*/

    public void testParamsNotLoaded_throwsException() {
        assertThrows(IllegalStateException.class, () -> mController.getParams());
    }

    public void testInitiateProvisioning_showsWifiPicker() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .build();
        initiateProvisioning(params);
        verify(mUi).requestWifiPick();
    }

    public void testInitiateProvisioning_useMobileData_showsWifiPicker() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setUseMobileData(true)
                .build();
        initiateProvisioning(params);
        verify(mUi).requestWifiPick();
    }

    public void testInitiateProvisioning_useMobileData_noWifiPicker() {
        when(mUtils.isMobileNetworkConnectedToInternet(mContext)).thenReturn(true);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setUseMobileData(true)
                .build();
        initiateProvisioning(params);
        verify(mUi, never()).requestWifiPick();
    }

    public void testInitiateProvisioning_connectedToWifiOrEthernet_noWifiPicker() {
        when(mUtils.isNetworkTypeConnected(mContext, ConnectivityManager.TYPE_WIFI,
                ConnectivityManager.TYPE_ETHERNET)).thenReturn(true);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .build();
        initiateProvisioning(params);
        verify(mUi, never()).requestWifiPick();
    }

    public void testInitiateProvisioning_noAdminDownloadInfo_noWifiPicker() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setDeviceAdminDownloadInfo(null)
                .build();
        initiateProvisioning(params);
        verify(mUi, never()).requestWifiPick();
    }

    public void testInitiateProvisioning_wifiInfo_noWifiPicker() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setWifiInfo(new WifiInfo.Builder().setSsid(TEST_WIFI_SSID).build())
                .build();
        initiateProvisioning(params);
        verify(mUi, never()).requestWifiPick();
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_orgDevice_exactExtras() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE,
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.size()).isEqualTo(5);
    }

    public void testGetAdditionalExtrasForGetProvisioningModeIntent_orgDevice_imeiPassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE,
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.getString(EXTRA_PROVISIONING_IMEI)).isEqualTo(TEST_IMEI);
    }

    public void testGetAdditionalExtrasForGetProvisioningModeIntent_orgDevice_serialNumberPassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE,
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.containsKey(EXTRA_PROVISIONING_SERIAL_NUMBER)).isTrue();
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_nonOrgDevice_adminBundlePassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(List.of(PROVISIONING_MODE_MANAGED_PROFILE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.keySet()).contains(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE);
        assertThat((PersistableBundle) bundle.getParcelable(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE))
                .isEqualTo(TEST_ADMIN_BUNDLE);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_nonOrgDevice_allowedModesPassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(List.of(PROVISIONING_MODE_MANAGED_PROFILE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.getIntegerArrayList(EXTRA_PROVISIONING_ALLOWED_PROVISIONING_MODES))
                .containsExactly(PROVISIONING_MODE_MANAGED_PROFILE);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_nonOrgDevice_hasExactlyTwoExtras() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(List.of(PROVISIONING_MODE_MANAGED_PROFILE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.size()).isEqualTo(2);
    }

    public void testGetAdditionalExtrasForGetProvisioningModeIntent_orgDevice_adminBundlePassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat((PersistableBundle) bundle.getParcelable(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE))
                .isEqualTo(TEST_ADMIN_BUNDLE);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManagedDevice_adminBundlePassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(List.of(PROVISIONING_MODE_FULLY_MANAGED_DEVICE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_DEVICE_OWNER)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.keySet()).contains(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE);
        assertThat((PersistableBundle) bundle.getParcelable(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE))
                .isEqualTo(TEST_ADMIN_BUNDLE);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManagedDevice_exactExtras() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_DEVICE_OWNER)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.size()).isEqualTo(5);
    }

    public void testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManagedDevice_imeiPassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_DEVICE_OWNER)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.getString(EXTRA_PROVISIONING_IMEI)).isEqualTo(TEST_IMEI);
    }

    public void testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManagedDevice_serialNumberPassed() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_DEVICE_OWNER)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.containsKey(EXTRA_PROVISIONING_SERIAL_NUMBER)).isTrue();
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManaged_hasOptOutExtra() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE,
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.getBoolean(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(false);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManaged_optOutExtraIsTrue() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_DEVICE_OWNER)
                .setDeviceOwnerPermissionGrantOptOut(true)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.getBoolean(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(true);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_managedProfile_optOutExtraIsFalseByDefault() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(List.of(PROVISIONING_MODE_MANAGED_PROFILE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.containsKey(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(false);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_fullyManaged_optOutExtraIsFalse() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(true)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_DEVICE_OWNER)
                .setDeviceOwnerPermissionGrantOptOut(false)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.getBoolean(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(false);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_managedProfileByo_optOutExtraNotPresent() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(
                                List.of(PROVISIONING_MODE_MANAGED_PROFILE_ON_PERSONAL_DEVICE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.containsKey(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(false);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_managedProfile_optOutExtraNotPresent() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(List.of(PROVISIONING_MODE_MANAGED_PROFILE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .setDeviceOwnerPermissionGrantOptOut(true)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.containsKey(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(false);
    }

    public void
            testGetAdditionalExtrasForGetProvisioningModeIntent_managedProfileByo_optOutExtraHasNoEffect() {
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setIsOrganizationOwnedProvisioning(false)
                .setAdminExtrasBundle(TEST_ADMIN_BUNDLE)
                .setAllowedProvisioningModes(
                        new ArrayList<>(
                                List.of(PROVISIONING_MODE_MANAGED_PROFILE_ON_PERSONAL_DEVICE)))
                .setInitiatorRequestedProvisioningModes(FLAG_SUPPORTED_MODES_PERSONALLY_OWNED)
                .setDeviceOwnerPermissionGrantOptOut(true)
                .build();
        initiateProvisioning(params);

        Bundle bundle = mController.getAdditionalExtrasForGetProvisioningModeIntent();

        assertThat(bundle.containsKey(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT))
                .isEqualTo(false);
    }

    public void testUpdateProvisioningParamsFromIntent_managedProfileModeWithAccountMigratedExtraTrue_setsParamToTrue() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE)
                .putExtra(EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION, true);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().keepAccountMigrated).isTrue();
    }

    public void testUpdateProvisioningParamsFromIntent_managedProfileModeWithAccountMigratedExtraFalse_setsParamToFalse() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE)
                .putExtra(EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION, false);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().keepAccountMigrated).isFalse();
    }

    public void testUpdateProvisioningParamsFromIntent_managedProfileMode_accountMigratedIsFalse() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().keepAccountMigrated).isFalse();
    }

    public void testUpdateProvisioningParamsFromIntent_managedDeviceModeWithAccountMigratedExtraTrue_accountMigratedIsFalse() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_FULLY_MANAGED_DEVICE)
                .putExtra(EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION, true);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().keepAccountMigrated).isFalse();
    }

    public void testUpdateProvisioningParamsFromIntent_managedProfileModeWithLeaveSystemAppsEnabledTrue_setsParamToTrue() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE)
                .putExtra(EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED, true);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().leaveAllSystemAppsEnabled).isTrue();
    }

    public void testUpdateProvisioningParamsFromIntent_managedProfileModeWithLeaveSystemAppsEnabledFalse_setsParamToFalse() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE)
                .putExtra(EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED, false);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().leaveAllSystemAppsEnabled).isFalse();
    }

    public void testUpdateProvisioningParamsFromIntent_managedProfileMode_leaveSystemAppsEnabledIsFalse() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().leaveAllSystemAppsEnabled).isFalse();
    }

    public void testUpdateProvisioningParamsFromIntent_managedDeviceModeWithLeaveSystemAppsEnabledTrue_paramIsFalse() {
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_FULLY_MANAGED_DEVICE)
                .putExtra(EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED, true);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_FULLY_MANAGED_DEVICE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().leaveAllSystemAppsEnabled).isFalse();
    }

    public void testUpdateProvisioningParamsFromIntent_adminBundlePassed_setsParam() {
        PersistableBundle testAdminExtrasBundle = new PersistableBundle();
        testAdminExtrasBundle.putInt("key1", 2);
        testAdminExtrasBundle.putString("key2", "value2");
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE)
                .putExtra(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE, testAdminExtrasBundle);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().adminExtrasBundle).isEqualTo(testAdminExtrasBundle);
    }

    public void testUpdateProvisioningParamsFromIntent_adminBundlePassedWithPreexistingAdminBundle_appendsValues() {
        PersistableBundle resultingAdminBundle = new PersistableBundle();
        resultingAdminBundle.putInt("key1", 2);
        resultingAdminBundle.putInt("someKey", 124);
        PersistableBundle existingAdminBundle = new PersistableBundle();
        existingAdminBundle.putInt("key2", 3);
        existingAdminBundle.putInt("someKey", 123);
        PersistableBundle expectedResult = new PersistableBundle();
        expectedResult.putInt("key1", 2);
        expectedResult.putInt("key2", 3);
        expectedResult.putInt("someKey", 124);
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE)
                .putExtra(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE, resultingAdminBundle);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAdminExtrasBundle(existingAdminBundle)
                .setKeepAccountMigrated(false)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().adminExtrasBundle.toString())
                .isEqualTo(expectedResult.toString());
    }

    public void testUpdateProvisioningParamsFromIntent_noAdminBundleResult_existingAdminBundleRetained() {
        PersistableBundle existingAdminBundle = new PersistableBundle();
        existingAdminBundle.putInt("key2", 3);
        existingAdminBundle.putInt("someKey", 123);
        Intent resultIntent = new Intent()
                .putExtra(EXTRA_PROVISIONING_MODE, PROVISIONING_MODE_MANAGED_PROFILE);
        final ProvisioningParams params = createProvisioningParamsBuilderForInitiateProvisioning()
                .setAdminExtrasBundle(existingAdminBundle)
                .setKeepAccountMigrated(false)
                .setAllowedProvisioningModes(new ArrayList<>(List.of(
                        PROVISIONING_MODE_MANAGED_PROFILE
                )))
                .build();
        initiateProvisioning(params);

        mController.updateProvisioningParamsFromIntent(resultIntent);

        assertThat(mController.getParams().adminExtrasBundle.toString())
                .isEqualTo(existingAdminBundle.toString());
    }

    public void testInitiateProvisioning_withActionProvisionManagedDevice_failsSilently()
            throws Exception {
        prepareMocksForDoIntent(/* skipEncryption= */ false);

        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE);
        });

        verify(mUi, never()).initiateUi(any());
        verify(mUi).abortProvisioning();
        verifyNoMoreInteractions(mUi);
    }

    private ProvisioningParams.Builder createProvisioningParamsBuilderForInitiateProvisioning() {
        return createProvisioningParamsBuilder()
                .setDeviceAdminDownloadInfo(PACKAGE_DOWNLOAD_INFO);
    }

    private void prepareMocksForManagedProfileIntent(boolean skipEncryption) throws Exception {
        final String action = ACTION_PROVISION_MANAGED_PROFILE;
        when(mIntent.getAction()).thenReturn(action);
        when(mUtils.findDeviceAdmin(TEST_MDM_PACKAGE, null, mContext, UserHandle.myUserId()))
                .thenReturn(TEST_MDM_COMPONENT_NAME);
        when(mSettingsFacade.isDeviceProvisioned(mContext)).thenReturn(true);
        when(mDevicePolicyManager.checkProvisioningPreCondition(action, TEST_MDM_PACKAGE))
                .thenReturn(CODE_OK);
        when(mMessageParser.parse(mIntent)).thenReturn(
                createParams(false, skipEncryption, null, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForNfcIntent(String action, boolean skipEncryption) throws Exception {
        when(mIntent.getAction()).thenReturn(ACTION_NDEF_DISCOVERED);
        when(mIntent.getComponent()).thenReturn(ComponentName.createRelative(MP_PACKAGE_NAME,
                ".PreProvisioningActivityViaNfc"));
        when(mDevicePolicyManager.checkProvisioningPreCondition(action, TEST_MDM_PACKAGE))
                .thenReturn(CODE_OK);
        mParams = createParamsBuilder(true, skipEncryption, TEST_WIFI_SSID, action,
                TEST_MDM_PACKAGE)
                .setIsNfc(true)
                .build();
        when(mMessageParser.parse(mIntent)).thenReturn(mParams);
    }

    private void prepareMocksForQrIntent(String action, boolean skipEncryption) throws Exception {
        when(mIntent.getAction())
                .thenReturn(ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE);
        when(mIntent.getComponent()).thenReturn(ComponentName.createRelative(MP_PACKAGE_NAME,
                ".PreProvisioningActivityViaTrustedApp"));
        when(mDevicePolicyManager.checkProvisioningPreCondition(action, TEST_MDM_PACKAGE))
                .thenReturn(CODE_OK);
        when(mMessageParser.parse(mIntent)).thenReturn(
                createParams(true, skipEncryption, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForDoIntent(boolean skipEncryption) throws Exception {
        final String action = ACTION_PROVISION_MANAGED_DEVICE;
        when(mIntent.getAction()).thenReturn(action);
        when(mDevicePolicyManager.checkProvisioningPreCondition(action, TEST_MDM_PACKAGE))
                .thenReturn(CODE_OK);
        when(mMessageParser.parse(mIntent)).thenReturn(
                createParams(false, skipEncryption, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private void prepareMocksForAfterEncryption(String action, boolean startedByTrustedSource)
            throws Exception {
        when(mIntent.getAction()).thenReturn(ACTION_RESUME_PROVISIONING);
        when(mIntent.getComponent()).thenReturn(ComponentName.createRelative(MP_PACKAGE_NAME,
                ".PreProvisioningActivityAfterEncryption"));
        when(mDevicePolicyManager.checkProvisioningPreCondition(action, TEST_MDM_PACKAGE))
                .thenReturn(CODE_OK);
        when(mMessageParser.parse(mIntent)).thenReturn(
                createParams(
                        startedByTrustedSource, false, TEST_WIFI_SSID, action, TEST_MDM_PACKAGE));
    }

    private ProvisioningParams.Builder createParamsBuilder(
            boolean startedByTrustedSource, boolean skipEncryption,
            String wifiSsid, String action, String packageName) {
        ProvisioningParams.Builder builder = ProvisioningParams.Builder.builder()
                .setStartedByTrustedSource(startedByTrustedSource)
                .setSkipEncryption(skipEncryption)
                .setProvisioningAction(action)
                .setDeviceAdminPackageName(packageName);
        if (!TextUtils.isEmpty(wifiSsid)) {
            builder.setWifiInfo(WifiInfo.Builder.builder().setSsid(wifiSsid).build());
        }
        return builder;
    }

    private ProvisioningParams createParams(boolean startedByTrustedSource, boolean skipEncryption,
            String wifiSsid, String action, String packageName) {
        return mParams = createParamsBuilder(
                startedByTrustedSource, skipEncryption, wifiSsid, action, packageName).build();
    }

    private void verifyInitiateProfileOwnerUi() {
        verify(mUi).initiateUi(any());
    }

    private void verifyInitiateDeviceOwnerUi() {
        verify(mUi).initiateUi(any());
    }

    private ProvisioningParams.Builder createProvisioningParamsBuilder() {
        return ProvisioningParams.Builder.builder()
                .setProvisioningAction(ACTION_PROVISION_MANAGED_DEVICE)
                .setStartedByTrustedSource(true)
                .setSkipEncryption(true)
                .setDeviceAdminComponentName(TEST_MDM_COMPONENT_NAME);
    }

    private void initiateProvisioning(ProvisioningParams params) {
        try {
            when(mMessageParser.parse(any(Intent.class))).thenReturn(params);
        } catch (IllegalProvisioningArgumentException e) {
            // will never happen
        }
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                mController.initiateProvisioning(mIntent, TEST_MDM_PACKAGE));
    }
}
