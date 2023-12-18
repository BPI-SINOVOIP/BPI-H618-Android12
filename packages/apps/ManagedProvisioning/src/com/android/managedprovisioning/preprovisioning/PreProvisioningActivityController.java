/*
 * Copyright 2016 The Android Open Source Project
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

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_FINANCED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.CODE_CANNOT_ADD_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.CODE_HAS_DEVICE_OWNER;
import static android.app.admin.DevicePolicyManager.CODE_MANAGED_USERS_NOT_SUPPORTED;
import static android.app.admin.DevicePolicyManager.CODE_NOT_SYSTEM_USER;
import static android.app.admin.DevicePolicyManager.CODE_OK;
import static android.app.admin.DevicePolicyManager.CODE_PROVISIONING_NOT_ALLOWED_FOR_NON_DEVELOPER_USERS;
import static android.app.admin.DevicePolicyManager.CODE_USER_SETUP_COMPLETED;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ALLOWED_PROVISIONING_MODES;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_IMEI;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SERIAL_NUMBER;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SKIP_EDUCATION_SCREENS;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_TRIGGER;
import static android.app.admin.DevicePolicyManager.FLAG_SUPPORTED_MODES_DEVICE_OWNER;
import static android.app.admin.DevicePolicyManager.FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED;
import static android.app.admin.DevicePolicyManager.PROVISIONING_MODE_MANAGED_PROFILE_ON_PERSONAL_DEVICE;
import static android.app.admin.DevicePolicyManager.PROVISIONING_TRIGGER_QR_CODE;
import static android.app.admin.DevicePolicyManager.PROVISIONING_TRIGGER_UNSPECIFIED;
import static android.nfc.NfcAdapter.ACTION_NDEF_DISCOVERED;

import static com.android.managedprovisioning.analytics.ProvisioningAnalyticsTracker.CANCELLED_BEFORE_PROVISIONING;
import static com.android.managedprovisioning.common.Globals.ACTION_RESUME_PROVISIONING;
import static com.android.managedprovisioning.model.ProvisioningParams.DEFAULT_EXTRA_PROVISIONING_KEEP_ACCOUNT_MIGRATED;
import static com.android.managedprovisioning.model.ProvisioningParams.DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED;
import static com.android.managedprovisioning.model.ProvisioningParams.FLOW_TYPE_ADMIN_INTEGRATED;
import static com.android.managedprovisioning.model.ProvisioningParams.FLOW_TYPE_LEGACY;

import static java.util.Objects.requireNonNull;

import android.accounts.Account;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.Activity;
import android.app.KeyguardManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.UserInfo;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.os.SystemClock;
import android.os.UserManager;
import android.service.persistentdata.PersistentDataBlockManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import androidx.activity.ComponentActivity;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.ViewModelProvider;

import com.android.internal.annotations.VisibleForTesting;
import com.android.managedprovisioning.ManagedProvisioningBaseApplication;
import com.android.managedprovisioning.ManagedProvisioningScreens;
import com.android.managedprovisioning.R;
import com.android.managedprovisioning.analytics.MetricsWriterFactory;
import com.android.managedprovisioning.analytics.ProvisioningAnalyticsTracker;
import com.android.managedprovisioning.common.GetProvisioningModeUtils;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.ManagedProvisioningSharedPreferences;
import com.android.managedprovisioning.common.PolicyComplianceUtils;
import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.CustomizationParams;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.model.ProvisioningParams.FlowType;
import com.android.managedprovisioning.preprovisioning.PreProvisioningViewModel.PreProvisioningViewModelFactory;

import java.util.List;

/**
 * Controller which contains business logic related to provisioning preparation.
 *
 * @see PreProvisioningActivity
 */
public class PreProvisioningActivityController {
    private final Context mContext;
    private final Ui mUi;
    private final Utils mUtils;
    private final PolicyComplianceUtils mPolicyComplianceUtils;
    private final GetProvisioningModeUtils mGetProvisioningModeUtils;
    private final SettingsFacade mSettingsFacade;

    // used system services
    private final DevicePolicyManager mDevicePolicyManager;
    private final UserManager mUserManager;
    private final PackageManager mPackageManager;
    private final KeyguardManager mKeyguardManager;
    private final PersistentDataBlockManager mPdbManager;
    private final ProvisioningAnalyticsTracker mProvisioningAnalyticsTracker;
    private final ManagedProvisioningSharedPreferences mSharedPreferences;

    private final PreProvisioningViewModel mViewModel;

    public PreProvisioningActivityController(
            @NonNull ComponentActivity activity,
            @NonNull Ui ui) {
        this(activity, ui,
                new Utils(), new SettingsFacade(),
                new ManagedProvisioningSharedPreferences(activity),
                new PolicyComplianceUtils(),
                new GetProvisioningModeUtils(),
                new ViewModelProvider(
                        activity,
                        new PreProvisioningViewModelFactory(
                                (ManagedProvisioningBaseApplication) activity.getApplication()))
                                        .get(PreProvisioningViewModel.class));
    }
    @VisibleForTesting
    PreProvisioningActivityController(
            @NonNull Context context,
            @NonNull Ui ui,
            @NonNull Utils utils,
            @NonNull SettingsFacade settingsFacade,
            @NonNull ManagedProvisioningSharedPreferences sharedPreferences,
            @NonNull PolicyComplianceUtils policyComplianceUtils,
            @NonNull GetProvisioningModeUtils getProvisioningModeUtils,
            @NonNull PreProvisioningViewModel viewModel) {
        mContext = requireNonNull(context, "Context must not be null");
        mUi = requireNonNull(ui, "Ui must not be null");
        mSettingsFacade = requireNonNull(settingsFacade);
        mUtils = requireNonNull(utils, "Utils must not be null");
        mPolicyComplianceUtils = requireNonNull(policyComplianceUtils,
                "PolicyComplianceUtils cannot be null");
        mGetProvisioningModeUtils = requireNonNull(getProvisioningModeUtils,
                "GetProvisioningModeUtils cannot be null");
        mSharedPreferences = requireNonNull(sharedPreferences);
        mViewModel = requireNonNull(viewModel);

        mDevicePolicyManager = mContext.getSystemService(DevicePolicyManager.class);
        mUserManager = mContext.getSystemService(UserManager.class);
        mPackageManager = mContext.getPackageManager();
        mKeyguardManager = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        mPdbManager = (PersistentDataBlockManager) mContext.getSystemService(
                Context.PERSISTENT_DATA_BLOCK_SERVICE);
        mProvisioningAnalyticsTracker = new ProvisioningAnalyticsTracker(
                MetricsWriterFactory.getMetricsWriter(mContext, mSettingsFacade),
                mSharedPreferences);
    }

    interface Ui {
        /**
         * Show an error message and cancel provisioning.
         * @param titleId resource id used to form the user facing error title
         * @param messageId resource id used to form the user facing error message
         * @param errorMessage an error message that gets logged for debugging
         */
        void showErrorAndClose(Integer titleId, int messageId, String errorMessage);

        /**
         * Request the user to encrypt the device.
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         */
        void requestEncryption(ProvisioningParams params);

        /**
         * Request the user to choose a wifi network.
         */
        void requestWifiPick();

        /**
         * Start provisioning.
         * @param params the {@link ProvisioningParams} object related to the ongoing provisioning
         */
        void startProvisioning(ProvisioningParams params);

        /**
         * Show an error dialog indicating that the current launcher does not support managed
         * profiles and ask the user to choose a different one.
         */
        void showCurrentLauncherInvalid();

        void showOwnershipDisclaimerScreen(ProvisioningParams params);

        void prepareFinancedDeviceFlow(ProvisioningParams params);

        void showFactoryResetDialog(Integer titleId, int messageId);

        void initiateUi(UiParams uiParams);

        /**
         *  Abort provisioning and close app
         */
        void abortProvisioning();

        void prepareAdminIntegratedFlow(ProvisioningParams params);
    }

    /**
     * Wrapper which holds information related to the consent screen.
     * <p>Does not implement {@link Object#equals(Object)}, {@link Object#hashCode()}
     * or {@link Object#toString()}.
     */
    public static class UiParams {
        /**
         * Admin application package name.
         */
        public String packageName;
        /**
         * Various organization-defined customizations, e.g. colors, organization name.
         */
        public CustomizationParams customization;
        /**
         * List of headings for the organization-provided terms and conditions.
         */
        public List<String> disclaimerHeadings;
        public boolean isDeviceManaged;
        /**
         * The original provisioning action, kept for backwards compatibility.
         */
        public String provisioningAction;
        public boolean isOrganizationOwnedProvisioning;
    }

    /**
     * Initiates Profile owner and device owner provisioning.
     * @param intent Intent that started provisioning.
     * @param callingPackage Package that started provisioning.
     */
    public void initiateProvisioning(Intent intent, String callingPackage) {
        mSharedPreferences.writeProvisioningStartedTimestamp(SystemClock.elapsedRealtime());
        mProvisioningAnalyticsTracker.logProvisioningSessionStarted(mContext);

        if (!tryParseParameters(intent)) {
            return;
        }

        ProvisioningParams params = mViewModel.getParams();
        if (!checkFactoryResetProtection(params, callingPackage)) {
            return;
        }

        if (!verifyActionAndCaller(intent, callingPackage)) {
            return;
        }

        // Check whether provisioning is allowed for the current action. This check needs to happen
        // before any actions that might affect the state of the device.
        // Note that checkDevicePolicyPreconditions takes care of calling
        // showProvisioningErrorAndClose. So we only need to show the factory reset dialog (if
        // applicable) and return.
        if (!checkDevicePolicyPreconditions()) {
            return;
        }

        if (!isIntentActionValid(intent.getAction())) {
            ProvisionLogger.loge(
                    ACTION_PROVISION_MANAGED_DEVICE + " is no longer a supported intent action.");
            mUi.abortProvisioning();
            return;
        }

        if (isDeviceOwnerProvisioning()) {
            // TODO: make a general test based on deviceAdminDownloadInfo field
            // PO doesn't ever initialize that field, so OK as a general case
            if (shouldShowWifiPicker(intent)) {
                // Have the user pick a wifi network if necessary.
                // It is not possible to ask the user to pick a wifi network if
                // the screen is locked.
                // TODO: remove this check once we know the screen will not be locked.
                if (mKeyguardManager.inKeyguardRestrictedInputMode()) {
                    // TODO: decide on what to do in that case; fail? retry on screen unlock?
                    ProvisionLogger.logi("Cannot pick wifi because the screen is locked.");
                } else if (canRequestWifiPick()) {
                    // we resume this method after a successful WiFi pick
                    // TODO: refactor as evil - logic should be less spread out
                    mUi.requestWifiPick();
                    return;
                } else {
                    mUi.showErrorAndClose(R.string.cant_set_up_device,
                            R.string.contact_your_admin_for_help,
                            "Cannot pick WiFi because there is no handler to the intent");
                }
            }
        }

        mViewModel.getTimeLogger().start();
        mProvisioningAnalyticsTracker.logPreProvisioningStarted(mContext, intent);
        mViewModel.onProvisioningInitiated();

        if (mUtils.checkAdminIntegratedFlowPreconditions(params)) {
            if (mUtils.shouldShowOwnershipDisclaimerScreen(params)) {
                mUi.showOwnershipDisclaimerScreen(params);
            } else {
                mUi.prepareAdminIntegratedFlow(params);
            }
            mViewModel.onAdminIntegratedFlowInitiated();
        } else if (mUtils.isFinancedDeviceAction(params.provisioningAction)) {
            mUi.prepareFinancedDeviceFlow(params);
        } else if (params.isNfc) {
            // TODO(b/177849035): Remove NFC-specific logic
            if (mUtils.shouldShowOwnershipDisclaimerScreen(params)) {
                mUi.showOwnershipDisclaimerScreen(params);
            } else {
                startNfcFlow();
            }
        } else if (isProfileOwnerProvisioning()) {
            startManagedProfileFlow();
        } else if (isDpcTriggeredManagedDeviceProvisioning(intent)) {
            // TODO(b/175678720): Fail provisioning if flow started by PROVISION_MANAGED_DEVICE
            startManagedDeviceFlow();
        }
    }

    private boolean isIntentActionValid(String action) {
        return !ACTION_PROVISION_MANAGED_DEVICE.equals(action);
    }

    void startNfcFlow() {
        ProvisionLogger.logi("Starting the NFC provisioning flow.");
        updateProvisioningFlowState(FLOW_TYPE_LEGACY);
        showUserConsentScreen();
    }

    private void startManagedProfileFlow() {
        ProvisionLogger.logi("Starting the managed profile flow.");
        showUserConsentScreen();
    }

    private void startManagedDeviceFlow() {
        ProvisionLogger.logi("Starting the legacy managed device flow.");
        showUserConsentScreen();
    }

    private boolean isDpcTriggeredManagedDeviceProvisioning(Intent intent) {
        return ACTION_PROVISION_MANAGED_DEVICE.equals(intent.getAction());
    }

    private boolean isNfcProvisioning(Intent intent) {
        return ACTION_NDEF_DISCOVERED.equals(intent.getAction());
    }

    private boolean isQrCodeProvisioning(Intent intent) {
        if (!ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE.equals(intent.getAction())) {
            return false;
        }
        final int provisioningTrigger = intent.getIntExtra(EXTRA_PROVISIONING_TRIGGER,
                PROVISIONING_TRIGGER_UNSPECIFIED);
        return provisioningTrigger == PROVISIONING_TRIGGER_QR_CODE;
    }

    private boolean shouldShowWifiPicker(Intent intent) {
        ProvisioningParams params = mViewModel.getParams();
        if (params.wifiInfo != null) {
            return false;
        }
        if (params.deviceAdminDownloadInfo == null) {
            return false;
        }
        if (mUtils.isNetworkTypeConnected(mContext, ConnectivityManager.TYPE_WIFI,
                ConnectivityManager.TYPE_ETHERNET)) {
            return false;
        }
        // we intentionally disregard whether mobile is connected for QR and NFC
        // provisioning. b/153442588 for context
        if (params.useMobileData
                && (isQrCodeProvisioning(intent) || isNfcProvisioning(intent))) {
            return false;
        }
        if (params.useMobileData) {
            return !mUtils.isMobileNetworkConnectedToInternet(mContext);
        }
        return true;
    }

    void showUserConsentScreen() {
        // Check whether provisioning is allowed for the current action
        if (!checkDevicePolicyPreconditions()) {
            return;
        }

        if (mViewModel.getParams().provisioningAction.equals(ACTION_PROVISION_MANAGED_PROFILE)
                && mViewModel.getParams().isOrganizationOwnedProvisioning) {
            mProvisioningAnalyticsTracker.logOrganizationOwnedManagedProfileProvisioning();
        }

        CustomizationParams customization =
                CustomizationParams.createInstance(mViewModel.getParams(), mContext, mUtils);

        // show UI so we can get user's consent to continue
        final String packageName = mViewModel.getParams().inferDeviceAdminPackageName();
        final UiParams uiParams = new UiParams();
        uiParams.customization = customization;
        uiParams.provisioningAction = mViewModel.getParams().provisioningAction;
        uiParams.packageName = packageName;
        uiParams.isDeviceManaged = mDevicePolicyManager.isDeviceManaged();
        uiParams.isOrganizationOwnedProvisioning =
                mViewModel.getParams().isOrganizationOwnedProvisioning;

        mUi.initiateUi(uiParams);
        mViewModel.onShowUserConsent();
    }

    boolean updateProvisioningParamsFromIntent(Intent resultIntent) {
        final int provisioningMode = resultIntent.getIntExtra(
                DevicePolicyManager.EXTRA_PROVISIONING_MODE, 0);
        if (!mViewModel.getParams().allowedProvisioningModes.contains(provisioningMode)) {
            ProvisionLogger.loge("Invalid provisioning mode chosen by the DPC: " + provisioningMode
                    + ", but expected one of "
                    + mViewModel.getParams().allowedProvisioningModes.toString());
            return false;
        }
        switch (provisioningMode) {
            case DevicePolicyManager.PROVISIONING_MODE_FULLY_MANAGED_DEVICE:
                updateParamsPostProvisioningModeDecision(
                        resultIntent,
                        ACTION_PROVISION_MANAGED_DEVICE,
                        /* isOrganizationOwnedProvisioning */ true,
                        /* updateAccountToMigrate */ false);
                return true;
            case DevicePolicyManager.PROVISIONING_MODE_MANAGED_PROFILE:
                updateParamsPostProvisioningModeDecision(
                        resultIntent,
                        ACTION_PROVISION_MANAGED_PROFILE,
                        mUtils.isOrganizationOwnedAllowed(mViewModel.getParams()),
                        /* updateAccountToMigrate */ true);
                return true;
            case PROVISIONING_MODE_MANAGED_PROFILE_ON_PERSONAL_DEVICE:
                updateParamsPostProvisioningModeDecision(
                        resultIntent,
                        ACTION_PROVISION_MANAGED_PROFILE,
                        /* isOrganizationOwnedProvisioning */ false,
                        /* updateAccountToMigrate */ true);
                return true;
            default:
                ProvisionLogger.logw("Unknown returned provisioning mode:"
                        + provisioningMode);
                return false;
        }
    }

    private void updateParamsPostProvisioningModeDecision(Intent resultIntent,
            String provisioningAction, boolean isOrganizationOwnedProvisioning,
            boolean updateAccountToMigrate) {
        ProvisioningParams.Builder builder = mViewModel.getParams().toBuilder();
        builder.setFlowType(FLOW_TYPE_ADMIN_INTEGRATED);
        builder.setProvisioningAction(provisioningAction);
        builder.setIsOrganizationOwnedProvisioning(isOrganizationOwnedProvisioning);
        maybeUpdateAdminExtrasBundle(builder, resultIntent);
        maybeUpdateSkipEducationScreens(builder, resultIntent);
        if (updateAccountToMigrate) {
            maybeUpdateAccountToMigrate(builder, resultIntent);
        }
        if (provisioningAction.equals(ACTION_PROVISION_MANAGED_PROFILE)) {
            maybeUpdateKeepAccountMigrated(builder, resultIntent);
            maybeUpdateLeaveAllSystemAppsEnabled(builder, resultIntent);
        }
        mViewModel.updateParams(builder.build());
    }

    private void maybeUpdateSkipEducationScreens(ProvisioningParams.Builder builder,
            Intent resultIntent) {
        if (resultIntent.hasExtra(EXTRA_PROVISIONING_SKIP_EDUCATION_SCREENS)) {
            builder.setSkipEducationScreens(resultIntent.getBooleanExtra(
                    EXTRA_PROVISIONING_SKIP_EDUCATION_SCREENS, /* defaultValue */ false));
        }
    }

    private void maybeUpdateAccountToMigrate(ProvisioningParams.Builder builder,
            Intent resultIntent) {
        if (resultIntent.hasExtra(EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE)) {
            final Account account = resultIntent.getParcelableExtra(
                    EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE);
            builder.setAccountToMigrate(account);
        }
    }

    /**
     * Appends the admin bundle in {@code resultIntent}, if provided, to the existing admin bundle,
     * if it exists, and stores the result in {@code builder}.
     */
    private void maybeUpdateAdminExtrasBundle(ProvisioningParams.Builder builder,
            Intent resultIntent) {
        if (resultIntent.hasExtra(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE)) {
            PersistableBundle resultBundle =
                    resultIntent.getParcelableExtra(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE);
            if (mViewModel.getParams().adminExtrasBundle != null) {
                PersistableBundle existingBundle =
                        new PersistableBundle(mViewModel.getParams().adminExtrasBundle);
                existingBundle.putAll(resultBundle);
                resultBundle = existingBundle;
            }
            builder.setAdminExtrasBundle(resultBundle);
        }
    }

    private void maybeUpdateKeepAccountMigrated(
            ProvisioningParams.Builder builder,
            Intent resultIntent) {
        if (resultIntent.hasExtra(EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION)) {
            final boolean keepAccountMigrated = resultIntent.getBooleanExtra(
                    EXTRA_PROVISIONING_KEEP_ACCOUNT_ON_MIGRATION,
                    DEFAULT_EXTRA_PROVISIONING_KEEP_ACCOUNT_MIGRATED);
            builder.setKeepAccountMigrated(keepAccountMigrated);
        }
    }

    private void maybeUpdateLeaveAllSystemAppsEnabled(
            ProvisioningParams.Builder builder,
            Intent resultIntent) {
        if (resultIntent.hasExtra(EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED)) {
            final boolean leaveAllSystemAppsEnabled = resultIntent.getBooleanExtra(
                    EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED,
                    DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED);
            builder.setLeaveAllSystemAppsEnabled(leaveAllSystemAppsEnabled);
        }
    }

    void updateProvisioningFlowState(@FlowType int flowType) {
        mViewModel.updateParams(mViewModel.getParams().toBuilder().setFlowType(flowType).build());
    }

    Bundle getAdditionalExtrasForGetProvisioningModeIntent() {
        Bundle bundle = new Bundle();
        if (shouldPassPersonalDataToAdminApp()) {
            final TelephonyManager telephonyManager = mContext.getSystemService(
                    TelephonyManager.class);
            bundle.putString(EXTRA_PROVISIONING_IMEI, telephonyManager.getImei());
            bundle.putString(EXTRA_PROVISIONING_SERIAL_NUMBER, Build.getSerial());
        }
        ProvisioningParams params = mViewModel.getParams();
        bundle.putParcelable(EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE,
                params.adminExtrasBundle);
        bundle.putIntegerArrayList(EXTRA_PROVISIONING_ALLOWED_PROVISIONING_MODES,
                params.allowedProvisioningModes);

        if (params.allowedProvisioningModes.contains(
                DevicePolicyManager.PROVISIONING_MODE_FULLY_MANAGED_DEVICE)) {
            bundle.putBoolean(EXTRA_PROVISIONING_SENSORS_PERMISSION_GRANT_OPT_OUT,
                    params.deviceOwnerPermissionGrantOptOut);
        }
        return bundle;
    }

    private boolean shouldPassPersonalDataToAdminApp() {
        ProvisioningParams params = mViewModel.getParams();
        return params.initiatorRequestedProvisioningModes == FLAG_SUPPORTED_MODES_ORGANIZATION_OWNED
                || params.initiatorRequestedProvisioningModes == FLAG_SUPPORTED_MODES_DEVICE_OWNER;
    }

    protected Intent createViewTermsIntent() {
        return new Intent(mContext, getTermsActivityClass())
                .putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, mViewModel.getParams());
    }

    private Class<? extends Activity> getTermsActivityClass() {
        return getBaseApplication().getActivityClassForScreen(ManagedProvisioningScreens.TERMS);
    }

    private ManagedProvisioningBaseApplication getBaseApplication() {
        return (ManagedProvisioningBaseApplication) mContext.getApplicationContext();
    }

    /**
     * Start provisioning for real. In profile owner case, double check that the launcher
     * supports managed profiles if necessary. In device owner case, possibly create a new user
     * before starting provisioning.
     */
    public void continueProvisioningAfterUserConsent() {
        mProvisioningAnalyticsTracker.logProvisioningAction(
                mContext, mViewModel.getParams().provisioningAction);

        // check if encryption is required
        if (isEncryptionRequired()) {
            if (mDevicePolicyManager.getStorageEncryptionStatus()
                    == DevicePolicyManager.ENCRYPTION_STATUS_UNSUPPORTED) {
                mUi.showErrorAndClose(R.string.cant_set_up_device,
                        R.string.device_doesnt_allow_encryption_contact_admin,
                        "This device does not support encryption, and "
                                + DevicePolicyManager.EXTRA_PROVISIONING_SKIP_ENCRYPTION
                                + " was not passed.");
            } else {
                mUi.requestEncryption(mViewModel.getParams());
                // we come back to this method after returning from encryption dialog
                // TODO: refactor as evil - logic should be less spread out
            }
            return;
        }

        if (isProfileOwnerProvisioning()) { // PO case
            // Check whether the current launcher supports managed profiles.
            if (!mUtils.currentLauncherSupportsManagedProfiles(mContext)) {
                mUi.showCurrentLauncherInvalid();
                // we come back to this method after returning from launcher dialog
                // TODO: refactor as evil - logic should be less spread out
                return;
            } else {
                // Cancel the boot reminder as provisioning has now started.
                mViewModel.getEncryptionController().cancelEncryptionReminder();
                stopTimeLogger();
                mUi.startProvisioning(mViewModel.getParams());
            }
        } else { // DO case
            // Cancel the boot reminder as provisioning has now started.
            mViewModel.getEncryptionController().cancelEncryptionReminder();
            stopTimeLogger();
            mUi.startProvisioning(mViewModel.getParams());
        }

        mViewModel.onProvisioningStartedAfterUserConsent();
    }

    /** @return False if condition preventing further provisioning */
    @VisibleForTesting
    boolean checkFactoryResetProtection(ProvisioningParams params, String callingPackage) {
        if (skipFactoryResetProtectionCheck(params, callingPackage)) {
            return true;
        }
        if (factoryResetProtected()) {
            mUi.showErrorAndClose(R.string.cant_set_up_device,
                    R.string.device_has_reset_protection_contact_admin,
                    "Factory reset protection blocks provisioning.");
            return false;
        }
        return true;
    }

    private boolean skipFactoryResetProtectionCheck(
            ProvisioningParams params, String callingPackage) {
        if (TextUtils.isEmpty(callingPackage)) {
            return false;
        }
        String persistentDataPackageName = mContext.getResources()
                .getString(com.android.internal.R.string.config_persistentDataPackageName);
        try {
            // Only skip the FRP check if the caller is the package responsible for maintaining FRP
            // - i.e. if this is a flow for restoring device owner after factory reset.
            PackageInfo callingPackageInfo = mPackageManager.getPackageInfo(callingPackage, 0);
            return callingPackageInfo != null
                    && callingPackageInfo.applicationInfo != null
                    && callingPackageInfo.applicationInfo.isSystemApp()
                    && !TextUtils.isEmpty(persistentDataPackageName)
                    && callingPackage.equals(persistentDataPackageName)
                    && params != null
                    && params.startedByTrustedSource;
        } catch (PackageManager.NameNotFoundException e) {
            ProvisionLogger.loge("Calling package not found.", e);
            return false;
        }
    }

    /** @return False if condition preventing further provisioning */
    @VisibleForTesting protected boolean checkDevicePolicyPreconditions() {
        ProvisioningParams params = mViewModel.getParams();
        int provisioningPreCondition = mDevicePolicyManager.checkProvisioningPreCondition(
                params.provisioningAction,
                params.inferDeviceAdminPackageName());
        // Check whether provisioning is allowed for the current action.
        if (provisioningPreCondition != CODE_OK) {
            mProvisioningAnalyticsTracker.logProvisioningNotAllowed(mContext,
                    provisioningPreCondition);
            showProvisioningErrorAndClose(
                    params.provisioningAction, provisioningPreCondition);
            return false;
        }
        return true;
    }

    /** @return False if condition preventing further provisioning */
    private boolean tryParseParameters(Intent intent) {
        try {
            // Read the provisioning params from the provisioning intent
            mViewModel.loadParamsIfNecessary(intent);
        } catch (IllegalProvisioningArgumentException e) {
            mUi.showErrorAndClose(R.string.cant_set_up_device, R.string.contact_your_admin_for_help,
                    e.getMessage());
            return false;
        }
        return true;
    }

    /** @return False if condition preventing further provisioning */
    @VisibleForTesting protected boolean verifyActionAndCaller(Intent intent,
            String callingPackage) {
        if (verifyActionAndCallerInner(intent, callingPackage)) {
            return true;
        } else {
            mUi.showErrorAndClose(R.string.cant_set_up_device, R.string.contact_your_admin_for_help,
                    "invalid intent or calling package");
            return false;
        }
    }

    private boolean verifyActionAndCallerInner(Intent intent, String callingPackage) {
        // If this is a resume after encryption or trusted intent, we verify the activity alias.
        // Otherwise, verify that the calling app is trying to set itself as Device/ProfileOwner
        if (ACTION_RESUME_PROVISIONING.equals(intent.getAction())) {
            return verifyActivityAlias(intent, "PreProvisioningActivityAfterEncryption");
        } else if (ACTION_NDEF_DISCOVERED.equals(intent.getAction())) {
            return verifyActivityAlias(intent, "PreProvisioningActivityViaNfc");
        } else if (ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE.equals(intent.getAction())
                || ACTION_PROVISION_FINANCED_DEVICE.equals(intent.getAction())) {
            return verifyActivityAlias(intent, "PreProvisioningActivityViaTrustedApp");
        } else {
            return verifyCaller(callingPackage);
        }
    }

    private boolean verifyActivityAlias(Intent intent, String activityAlias) {
        ComponentName componentName = intent.getComponent();
        if (componentName == null || componentName.getClassName() == null) {
            ProvisionLogger.loge("null class in component when verifying activity alias "
                    + activityAlias);
            return false;
        }

        if (!componentName.getClassName().endsWith(activityAlias)) {
            ProvisionLogger.loge("Looking for activity alias " + activityAlias + ", but got "
                    + componentName.getClassName());
            return false;
        }

        return true;
    }

    /**
     * Verify that the caller is trying to set itself as owner.
     * @return false if the caller is trying to set a different package as owner.
     */
    private boolean verifyCaller(@NonNull String callingPackage) {
        if (callingPackage == null) {
            ProvisionLogger.loge("Calling package is null. Was startActivityForResult used to "
                    + "start this activity?");
            return false;
        }

        if (!callingPackage.equals(mViewModel.getParams().inferDeviceAdminPackageName())) {
            ProvisionLogger.loge("Permission denied, "
                    + "calling package tried to set a different package as owner. ");
            return false;
        }

        return true;
    }

    /**
     * Returns whether the device needs encryption.
     */
    private boolean isEncryptionRequired() {
        return !mViewModel.getParams().skipEncryption && mUtils.isEncryptionRequired();
    }

    /**
     * Returns whether the device is frp protected during setup wizard.
     */
    private boolean factoryResetProtected() {
        // If we are started during setup wizard, check for factory reset protection.
        // If the device is already setup successfully, do not check factory reset protection.
        if (mSettingsFacade.isDeviceProvisioned(mContext)) {
            ProvisionLogger.logd("Device is provisioned, FRP not required.");
            return false;
        }

        if (mPdbManager == null) {
            ProvisionLogger.logd("Reset protection not supported.");
            return false;
        }
        int size = mPdbManager.getDataBlockSize();
        ProvisionLogger.logd("Data block size: " + size);
        return size > 0;
    }

    /**
     * Returns whether activity to pick wifi can be requested or not.
     */
    private boolean canRequestWifiPick() {
        return mPackageManager.resolveActivity(mUtils.getWifiPickIntent(), 0) != null;
    }

    /**
     * Returns whether the provisioning process is a profile owner provisioning process.
     */
    public boolean isProfileOwnerProvisioning() {
        return mUtils.isProfileOwnerAction(mViewModel.getParams().provisioningAction);
    }

    /**
     * Returns whether the provisioning process is a device owner provisioning process.
     */
    public boolean isDeviceOwnerProvisioning() {
        return mUtils.isDeviceOwnerAction(mViewModel.getParams().provisioningAction);
    }


    @Nullable
    public ProvisioningParams getParams() {
        return mViewModel.getParams();
    }

    /**
     * Notifies the time logger to stop.
     */
    public void stopTimeLogger() {
        mViewModel.getTimeLogger().stop();
    }

    /**
     * Log if PreProvisioning was cancelled.
     */
    public void logPreProvisioningCancelled() {
        mProvisioningAnalyticsTracker.logProvisioningCancelled(mContext,
                CANCELLED_BEFORE_PROVISIONING);
    }

    /**
     * Logs the provisioning flow type.
     */
    public void logProvisioningFlowType() {
        mProvisioningAnalyticsTracker.logProvisioningFlowType(mViewModel.getParams());
    }

    /**
     * Removes a user profile. If we are in COMP case, and were blocked by having to delete a user,
     * resumes COMP provisioning.
     */
    public void removeUser(int userProfileId) {
        // There is a possibility that the DO has set the disallow remove managed profile user
        // restriction, but is initiating the provisioning. In this case, we still want to remove
        // the managed profile.
        // We know that we can remove the managed profile because we checked
        // DevicePolicyManager.checkProvisioningPreCondition
        mUserManager.removeUserEvenWhenDisallowed(userProfileId);
    }

    SettingsFacade getSettingsFacade() {
        return mSettingsFacade;
    }

    public PolicyComplianceUtils getPolicyComplianceUtils() {
        return mPolicyComplianceUtils;
    }

    public GetProvisioningModeUtils getGetProvisioningModeUtils() {
        return mGetProvisioningModeUtils;
    }

    void onReturnFromProvisioning() {
        mViewModel.onReturnFromProvisioning();
    }

    LiveData<Integer> getState() {
        return mViewModel.getState();
    }

    private void showProvisioningErrorAndClose(String action, int provisioningPreCondition) {
        // Try to show an error message explaining why provisioning is not allowed.
        switch (action) {
            case ACTION_PROVISION_MANAGED_PROFILE:
                showManagedProfileErrorAndClose(provisioningPreCondition);
                return;
            case ACTION_PROVISION_MANAGED_DEVICE:
                showDeviceOwnerErrorAndClose(provisioningPreCondition);
                return;
        }
        // This should never be the case, as showProvisioningError is always called after
        // verifying the supported provisioning actions.
    }

    private void showManagedProfileErrorAndClose(int provisioningPreCondition) {
        UserInfo userInfo = mUserManager.getUserInfo(mUserManager.getUserHandle());
        ProvisionLogger.logw("DevicePolicyManager.checkProvisioningPreCondition returns code: "
                + provisioningPreCondition);
        // If this is organization-owned provisioning, do not show any other error dialog, just
        // show the factory reset dialog and return.
        // This cannot be abused by regular apps to force a factory reset because
        // isOrganizationOwnedProvisioning is only set to true if the provisioning action was
        // from a trusted source. See Utils.isOrganizationOwnedProvisioning where we check for
        // ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE which is guarded by the
        // DISPATCH_PROVISIONING_MESSAGE system|privileged permission.
        if (mUtils.isOrganizationOwnedAllowed(mViewModel.getParams())) {
            ProvisionLogger.loge(
                    "Provisioning preconditions failed for organization-owned provisioning.");
            mUi.showFactoryResetDialog(R.string.cant_set_up_device,
                    R.string.contact_your_admin_for_help);
            return;
        }
        switch (provisioningPreCondition) {
            case CODE_MANAGED_USERS_NOT_SUPPORTED:
                mUi.showErrorAndClose(R.string.cant_add_work_profile,
                        R.string.work_profile_cant_be_added_contact_admin,
                        "Exiting managed profile provisioning, managed profiles "
                                + "feature is not available");
                break;
            case CODE_CANNOT_ADD_MANAGED_PROFILE:
                if (!userInfo.canHaveProfile()) {
                    mUi.showErrorAndClose(R.string.cant_add_work_profile,
                            R.string.work_profile_cant_be_added_contact_admin,
                            "Exiting managed profile provisioning, calling user cannot "
                                    + "have managed profiles");
                } else if (!canAddManagedProfile()) {
                    mUi.showErrorAndClose(R.string.cant_add_work_profile,
                            R.string.work_profile_cant_be_added_contact_admin,
                            "Exiting managed profile provisioning, a managed profile "
                                    + "already exists");
                } else {
                    mUi.showErrorAndClose(R.string.cant_add_work_profile,
                            R.string.work_profile_cant_be_added_contact_admin,
                            "Exiting managed profile provisioning, cannot add more managed "
                                    + "profiles");
                }
                break;
            case CODE_PROVISIONING_NOT_ALLOWED_FOR_NON_DEVELOPER_USERS:
                mUi.showErrorAndClose(R.string.cant_add_work_profile,
                        R.string.work_profile_cant_be_added_contact_admin,
                        "Exiting managed profile provisioning, "
                                + "provisioning not allowed by OEM");
                break;
            default:
                mUi.showErrorAndClose(R.string.cant_add_work_profile,
                        R.string.contact_your_admin_for_help,
                        "Managed profile provisioning not allowed for an unknown "
                                + "reason, code: " + provisioningPreCondition);
        }
    }

    private boolean canAddManagedProfile() {
        return mUserManager.canAddMoreManagedProfiles(
                mContext.getUserId(), /* allowedToRemoveOne= */ false);
    }

    private void showDeviceOwnerErrorAndClose(int provisioningPreCondition) {
        switch (provisioningPreCondition) {
            case CODE_HAS_DEVICE_OWNER:
            case CODE_USER_SETUP_COMPLETED:
                mUi.showErrorAndClose(R.string.device_already_set_up,
                        R.string.if_questions_contact_admin, "Device already provisioned.");
                return;
            case CODE_NOT_SYSTEM_USER:
                mUi.showErrorAndClose(R.string.cant_set_up_device,
                        R.string.contact_your_admin_for_help,
                        "Device owner can only be set up for USER_SYSTEM.");
                return;
            case CODE_PROVISIONING_NOT_ALLOWED_FOR_NON_DEVELOPER_USERS:
                mUi.showErrorAndClose(R.string.cant_set_up_device,
                        R.string.contact_your_admin_for_help,
                        "Provisioning not allowed by OEM");
                return;
        }
        mUi.showErrorAndClose(R.string.cant_set_up_device, R.string.contact_your_admin_for_help,
                "Device Owner provisioning not allowed for an unknown reason.");
    }
}
