/*
 * Copyright 2016, The Android Open Source Project
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

import static android.content.res.Configuration.UI_MODE_NIGHT_MASK;
import static android.content.res.Configuration.UI_MODE_NIGHT_YES;
import static android.nfc.NfcAdapter.ACTION_NDEF_DISCOVERED;

import static com.android.managedprovisioning.model.ProvisioningParams.FLOW_TYPE_LEGACY;
import static com.android.managedprovisioning.preprovisioning.PreProvisioningViewModel.STATE_PREPROVISIONING_INITIALIZING;
import static com.android.managedprovisioning.preprovisioning.PreProvisioningViewModel.STATE_SHOWING_USER_CONSENT;
import static com.android.managedprovisioning.provisioning.Constants.PROVISIONING_SERVICE_INTENT;

import static com.google.android.setupcompat.util.WizardManagerHelper.EXTRA_IS_SETUP_FLOW;

import android.app.Activity;
import android.app.BackgroundServiceStartNotAllowedException;
import android.app.DialogFragment;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.VisibleForTesting;

import com.android.managedprovisioning.ManagedProvisioningScreens;
import com.android.managedprovisioning.R;
import com.android.managedprovisioning.analytics.MetricsWriterFactory;
import com.android.managedprovisioning.analytics.ProvisioningAnalyticsTracker;
import com.android.managedprovisioning.common.AccessibilityContextMenuMaker;
import com.android.managedprovisioning.common.GetProvisioningModeUtils;
import com.android.managedprovisioning.common.Globals;
import com.android.managedprovisioning.common.LogoUtils;
import com.android.managedprovisioning.common.ManagedProvisioningSharedPreferences;
import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.common.SetupGlifLayoutActivity;
import com.android.managedprovisioning.common.SimpleDialog;
import com.android.managedprovisioning.common.ThemeHelper;
import com.android.managedprovisioning.common.ThemeHelper.DefaultNightModeChecker;
import com.android.managedprovisioning.common.ThemeHelper.DefaultSetupWizardBridge;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.preprovisioning.PreProvisioningActivityController.UiParams;
import com.android.managedprovisioning.provisioning.AdminIntegratedFlowPrepareActivity;
import com.android.managedprovisioning.provisioning.ProvisioningActivity;
import com.android.managedprovisioning.provisioning.ProvisioningService;

import com.google.android.setupcompat.util.WizardManagerHelper;

public class PreProvisioningActivity extends SetupGlifLayoutActivity implements
        SimpleDialog.SimpleDialogListener, PreProvisioningActivityController.Ui {

    private static final int ENCRYPT_DEVICE_REQUEST_CODE = 1;
    @VisibleForTesting
    protected static final int PROVISIONING_REQUEST_CODE = 2;
    private static final int WIFI_REQUEST_CODE = 3;
    private static final int CHANGE_LAUNCHER_REQUEST_CODE = 4;
    private static final int ORGANIZATION_OWNED_LANDING_PAGE_REQUEST_CODE = 5;
    private static final int GET_PROVISIONING_MODE_REQUEST_CODE = 6;
    private static final int FINANCED_DEVICE_PREPARE_REQUEST_CODE = 7;
    private static final int ADMIN_INTEGRATED_FLOW_PREPARE_REQUEST_CODE = 8;

    // Note: must match the constant defined in HomeSettings
    private static final String EXTRA_SUPPORT_MANAGED_PROFILES = "support_managed_profiles";

    private static final String ERROR_AND_CLOSE_DIALOG = "PreProvErrorAndCloseDialog";
    private static final String BACK_PRESSED_DIALOG_RESET = "PreProvBackPressedDialogReset";
    private static final String BACK_PRESSED_DIALOG_CLOSE_ACTIVITY =
            "PreProvBackPressedDialogCloseActivity";
    private static final String LAUNCHER_INVALID_DIALOG = "PreProvCurrentLauncherInvalidDialog";

    private PreProvisioningActivityController mController;
    private ControllerProvider mControllerProvider;
    private final AccessibilityContextMenuMaker mContextMenuMaker;
    private PreProvisioningActivityBridge mBridge;
    private boolean mShouldForwardTransition;

    private static final String ERROR_DIALOG_RESET = "ErrorDialogReset";

    public PreProvisioningActivity() {
        this(activity ->
                new PreProvisioningActivityController(activity, activity),
                null,
                new Utils(),
                new SettingsFacade(),
                new ThemeHelper(
                    new DefaultNightModeChecker(),
                    new DefaultSetupWizardBridge()));
    }

    @VisibleForTesting
    public PreProvisioningActivity(
            ControllerProvider controllerProvider,
            AccessibilityContextMenuMaker contextMenuMaker, Utils utils,
            SettingsFacade settingsFacade, ThemeHelper themeHelper) {
        super(utils, settingsFacade, themeHelper);
        mControllerProvider = controllerProvider;
        mContextMenuMaker =
                contextMenuMaker != null ? contextMenuMaker : new AccessibilityContextMenuMaker(
                        this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO(b/192074477): Remove deferred setup-specific logic after the managed account flow
        //  starts ManagedProvisioning with the isSetupFlow extra
        // TODO(b/178822333): Remove NFC-specific logic after adding support for the
        //  admin-integrated flow
        // This temporary fix only works when called before super.onCreate
        if (mSettingsFacade.isDeferredSetup(getApplicationContext()) || isNfcSetup()) {
            getIntent().putExtra(EXTRA_IS_SETUP_FLOW, true);
        }

        super.onCreate(savedInstanceState);
        mController = mControllerProvider.getInstance(this);
        mBridge = createBridge();
        mController.getState().observe(this, this::onStateChanged);
        logMetrics();
    }

    @Override
    protected void onStart() {
        super.onStart();
        try {
            getApplicationContext().startService(PROVISIONING_SERVICE_INTENT);
        } catch (BackgroundServiceStartNotAllowedException e) {
            ProvisionLogger.loge(e);
        }
    }

    private boolean isNfcSetup() {
        return ACTION_NDEF_DISCOVERED.equals(getIntent().getAction());
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mShouldForwardTransition) {
            overridePendingTransition(R.anim.sud_slide_next_in, R.anim.sud_slide_next_out);
            mShouldForwardTransition = false;
        }
    }

    protected PreProvisioningActivityBridge createBridge() {
        return new PreProvisioningActivityBridgeImpl(
                /* activity= */ this,
                mUtils,
                PreProvisioningActivity.this::initializeLayoutParams,
                createBridgeCallbacks(),
                getThemeHelper());
    }

    protected final PreProvisioningActivityBridgeCallbacks createBridgeCallbacks() {
        return new PreProvisioningActivityBridgeCallbacks() {
            @Override
            public void onTermsAccepted() {
                mController.continueProvisioningAfterUserConsent();
            }

            @Override
            public void onTermsButtonClicked() {
                getTransitionHelper()
                        .startActivityWithTransition(PreProvisioningActivity.this,
                                mController.createViewTermsIntent());
            }
        };
    }

    private void onStateChanged(Integer state) {
        switch (state) {
            case STATE_PREPROVISIONING_INITIALIZING:
                mController.initiateProvisioning(getIntent(), getCallingPackage());
                break;
            case STATE_SHOWING_USER_CONSENT:
                mController.showUserConsentScreen();
                break;
        }
    }

    @Override
    public void finish() {
        // The user has backed out of provisioning, so we perform the necessary clean up steps.
        LogoUtils.cleanUp(this);
        ProvisioningParams params = mController.getParams();
        if (params != null) {
            params.cleanUp();
        }
        getEncryptionController().cancelEncryptionReminder();
        getApplicationContext().stopService(PROVISIONING_SERVICE_INTENT);
        super.finish();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case ENCRYPT_DEVICE_REQUEST_CODE:
                if (resultCode == RESULT_CANCELED) {
                    ProvisionLogger.loge("User canceled device encryption.");
                }
                break;
            case PROVISIONING_REQUEST_CODE:
                mController.onReturnFromProvisioning();
                setResult(resultCode);
                getTransitionHelper().finishActivity(this);
                break;
            case CHANGE_LAUNCHER_REQUEST_CODE:
                mController.continueProvisioningAfterUserConsent();
                break;
            case WIFI_REQUEST_CODE:
                if (resultCode == RESULT_CANCELED) {
                    ProvisionLogger.loge("User canceled wifi picking.");
                    setResult(resultCode);
                    getTransitionHelper().finishActivity(this);
                } else {
                    if (resultCode == RESULT_OK) {
                        ProvisionLogger.logd("Wifi request result is OK");
                    }
                    mController.initiateProvisioning(getIntent(), getCallingPackage());
                }
                break;
            case ORGANIZATION_OWNED_LANDING_PAGE_REQUEST_CODE:
            case ADMIN_INTEGRATED_FLOW_PREPARE_REQUEST_CODE:
                if (resultCode == RESULT_OK) {
                    // TODO(b/177849035): Remove NFC-specific logic
                    if (mController.getParams().isNfc) {
                        mController.startNfcFlow();
                    } else {
                        handleAdminIntegratedFlowPreparerResult();
                    }
                } else {
                    ProvisionLogger.loge(
                            "Provisioning was aborted in the preparation stage, "
                                    + "requestCode = " + requestCode);
                    if (isDpcInstalled()
                            && mUtils.isOrganizationOwnedAllowed(mController.getParams())) {
                        showFactoryResetDialog(R.string.cant_set_up_device,
                                R.string.contact_your_admin_for_help);
                    } else {
                        showErrorAndClose(
                                R.string.cant_set_up_device,
                                R.string.contact_your_admin_for_help,
                                "Failed provisioning device.");
                    }
                }
                break;
            case GET_PROVISIONING_MODE_REQUEST_CODE:
                mShouldForwardTransition = true;
                if (resultCode == RESULT_OK) {
                    if(data != null && mController.updateProvisioningParamsFromIntent(data)) {
                        mController.showUserConsentScreen();
                    } else {
                        ProvisionLogger.loge(
                                "Invalid data object returned from GET_PROVISIONING_MODE.");
                        if (mUtils.isOrganizationOwnedAllowed(mController.getParams())) {
                            showFactoryResetDialog(R.string.cant_set_up_device,
                                    R.string.contact_your_admin_for_help);
                        } else {
                            showErrorAndClose(
                                    R.string.cant_set_up_device,
                                    R.string.contact_your_admin_for_help,
                                    "Failed provisioning personally-owned device.");
                        }
                    }
                } else {
                    ProvisionLogger.loge("Invalid result code from GET_PROVISIONING_MODE. Expected "
                            + RESULT_OK + " but got " + resultCode + ".");
                    if (mUtils.isOrganizationOwnedAllowed(mController.getParams())) {
                        showFactoryResetDialog(R.string.cant_set_up_device,
                                R.string.contact_your_admin_for_help);
                    } else {
                        showErrorAndClose(
                                R.string.cant_set_up_device,
                                R.string.contact_your_admin_for_help,
                                "Failed to provision personally-owned device.");
                    }
                }
                break;
            case FINANCED_DEVICE_PREPARE_REQUEST_CODE:
                if (resultCode == RESULT_OK) {
                    startFinancedDeviceFlow();
                } else {
                    setResult(resultCode);
                    getTransitionHelper().finishActivity(this);
                }
                break;
            default:
                ProvisionLogger.logw("Unknown result code :" + resultCode);
                break;
        }
    }

    private boolean isDpcInstalled() {
        String adminPackageName = mController.getParams().inferDeviceAdminPackageName();
        return mUtils.isPackageInstalled(adminPackageName, getPackageManager());
    }

    private void handleAdminIntegratedFlowPreparerResult() {
        if (isDpcInstalled()) {
            startAdminIntegratedFlowPostDpcInstall();
        } else {
            String adminPackageName = mController.getParams().inferDeviceAdminPackageName();
            showErrorAndClose(
                    R.string.cant_set_up_device,
                    R.string.contact_your_admin_for_help,
                    "Package name " + adminPackageName + " is not installed.");
        }
    }

    @Override
    public void showErrorAndClose(Integer titleId, int messageId, String logText) {
        ProvisionLogger.loge(logText);

        SimpleDialog.Builder dialogBuilder = new SimpleDialog.Builder()
                .setTitle(titleId)
                .setMessage(messageId)
                .setCancelable(false)
                .setPositiveButtonMessage(R.string.device_owner_error_ok);
        showDialog(dialogBuilder, ERROR_AND_CLOSE_DIALOG);
    }

    @Override
    public void onNegativeButtonClick(DialogFragment dialog) {
        switch (dialog.getTag()) {
            case BACK_PRESSED_DIALOG_CLOSE_ACTIVITY:
            case BACK_PRESSED_DIALOG_RESET:
                // user chose to continue. Do nothing
                break;
            case LAUNCHER_INVALID_DIALOG:
                dialog.dismiss();
                break;
            default:
                SimpleDialog.throwButtonClickHandlerNotImplemented(dialog);
        }
    }

    @Override
    public void onPositiveButtonClick(DialogFragment dialog) {
        switch (dialog.getTag()) {
            case ERROR_AND_CLOSE_DIALOG:
            case BACK_PRESSED_DIALOG_CLOSE_ACTIVITY:
                onProvisioningAborted();
                break;
            case BACK_PRESSED_DIALOG_RESET:
                mUtils.factoryReset(this, "Provisioning cancelled by user on consent screen");
                onProvisioningAborted();
                break;
            case LAUNCHER_INVALID_DIALOG:
                requestLauncherPick();
                break;
            case ERROR_DIALOG_RESET:
                getUtils().factoryReset(this, "Error during preprovisioning");
                setResult(Activity.RESULT_CANCELED);
                getTransitionHelper().finishActivity(this);
                break;
            default:
                SimpleDialog.throwButtonClickHandlerNotImplemented(dialog);
        }
    }

    private void onProvisioningAborted() {
        setResult(Activity.RESULT_CANCELED);
        mController.logPreProvisioningCancelled();
        getTransitionHelper().finishActivity(this);
    }

    @Override
    public void requestEncryption(ProvisioningParams params) {
        Intent encryptIntent = new Intent(this,
                getActivityForScreen(ManagedProvisioningScreens.ENCRYPT));
        WizardManagerHelper.copyWizardManagerExtras(getIntent(), encryptIntent);
        encryptIntent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        getTransitionHelper().startActivityForResultWithTransition(
                this, encryptIntent, ENCRYPT_DEVICE_REQUEST_CODE);
    }

    @Override
    public void requestWifiPick() {
        final Intent intent = mUtils.getWifiPickIntent();
        WizardManagerHelper.copyWizardManagerExtras(getIntent(), intent);
        getTransitionHelper()
                .startActivityForResultWithTransition(this, intent, WIFI_REQUEST_CODE);
    }

    @Override
    public void showCurrentLauncherInvalid() {
        SimpleDialog.Builder dialogBuilder = new SimpleDialog.Builder()
                .setCancelable(false)
                .setTitle(R.string.change_device_launcher)
                .setMessage(R.string.launcher_app_cant_be_used_by_work_profile)
                .setNegativeButtonMessage(R.string.cancel_provisioning)
                .setPositiveButtonMessage(R.string.pick_launcher);
        showDialog(dialogBuilder, LAUNCHER_INVALID_DIALOG);
    }

    @Override
    public void abortProvisioning() {
        onProvisioningAborted();
    }

    @Override
    public void prepareAdminIntegratedFlow(ProvisioningParams params) {
        if (AdminIntegratedFlowPrepareActivity.shouldRunPrepareActivity(mUtils, this, params)) {
            Intent intent = new Intent(this,
                    getActivityForScreen(ManagedProvisioningScreens.ADMIN_INTEGRATED_PREPARE));
            WizardManagerHelper.copyWizardManagerExtras(getIntent(), intent);
            intent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
            getTransitionHelper().startActivityForResultWithTransition(
                    this, intent, ADMIN_INTEGRATED_FLOW_PREPARE_REQUEST_CODE);
        } else {
            handleAdminIntegratedFlowPreparerResult();
        }
    }

    private void requestLauncherPick() {
        Intent changeLauncherIntent = new Intent(Settings.ACTION_HOME_SETTINGS);
        changeLauncherIntent.putExtra(EXTRA_SUPPORT_MANAGED_PROFILES, true);
        getTransitionHelper().startActivityForResultWithTransition(
                this, changeLauncherIntent, CHANGE_LAUNCHER_REQUEST_CODE);
    }

    /**
     * Starts {@link ProvisioningActivity}.
     */
    public void startProvisioning(ProvisioningParams params) {
        Intent intent = new Intent(this,
                getActivityForScreen(ManagedProvisioningScreens.PROVISIONING));
        WizardManagerHelper.copyWizardManagerExtras(getIntent(), intent);
        intent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        getTransitionHelper().startActivityForResultWithTransition(
                this, intent, PROVISIONING_REQUEST_CODE);
    }

    // TODO: The below group of methods do not belong in the activity.
    // Move them to the controller instead.
    /**
     * Starts either the admin-integrated or the legacy flow, depending on the device state and
     * DPC capabilities.
     */
    private void startAdminIntegratedFlowPostDpcInstall() {
        boolean canPerformAdminIntegratedFlow = mUtils.canPerformAdminIntegratedFlow(
                this,
                mController.getParams(),
                mController.getPolicyComplianceUtils(),
                mController.getGetProvisioningModeUtils());
        if (canPerformAdminIntegratedFlow) {
            startAdminIntegratedFlowWithoutPredeterminedMode();
        } else {
            ProvisionLogger.loge("The admin app does not have handlers for both "
                    + "ACTION_GET_PROVISIONING_MODE and ACTION_ADMIN_POLICY_COMPLIANCE "
                    + "intent actions.");
            if (mUtils.isOrganizationOwnedAllowed(mController.getParams())) {
                showFactoryResetDialog(R.string.cant_set_up_device,
                        R.string.contact_your_admin_for_help);
            } else {
                showErrorAndClose(
                        R.string.cant_set_up_device,
                        R.string.contact_your_admin_for_help,
                        "Failed provisioning personally-owned device.");
            }
        }
        mController.logProvisioningFlowType();
    }

    private void startAdminIntegratedFlowWithoutPredeterminedMode() {
        ProvisionLogger.logi("Starting the admin-integrated flow.");
        GetProvisioningModeUtils provisioningModeUtils = mController.getGetProvisioningModeUtils();
        Bundle additionalExtras = mController.getAdditionalExtrasForGetProvisioningModeIntent();
        provisioningModeUtils.startGetProvisioningModeActivityIfResolved(
                this, mController.getParams(), additionalExtras,
                GET_PROVISIONING_MODE_REQUEST_CODE, getTransitionHelper());
    }

    private void startFinancedDeviceFlow() {
        ProvisionLogger.logi("Starting the financed device flow.");
        mController.updateProvisioningFlowState(FLOW_TYPE_LEGACY);
        mController.continueProvisioningAfterUserConsent();
    }

    @Override
    public void showFactoryResetDialog(Integer titleId, int messageId) {
        SimpleDialog.Builder dialogBuilder = new SimpleDialog.Builder()
                .setTitle(titleId)
                .setMessage(messageId)
                .setCancelable(false)
                .setPositiveButtonMessage(R.string.reset);

        showDialog(dialogBuilder, ERROR_DIALOG_RESET);
    }

    @Override
    public void initiateUi(UiParams uiParams) {
        mBridge.initiateUi(uiParams);
    }

    @Override
    public void showOwnershipDisclaimerScreen(ProvisioningParams params) {
        Intent intent = new Intent(this,
                getActivityForScreen(ManagedProvisioningScreens.LANDING));
        WizardManagerHelper.copyWizardManagerExtras(getIntent(), intent);
        intent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        getTransitionHelper().startActivityForResultWithTransition(
                this, intent, ORGANIZATION_OWNED_LANDING_PAGE_REQUEST_CODE);
    }

    @Override
    public void prepareFinancedDeviceFlow(ProvisioningParams params) {
        Intent intent = new Intent(this,
                getActivityForScreen(ManagedProvisioningScreens.FINANCED_DEVICE_LANDING));
        WizardManagerHelper.copyWizardManagerExtras(getIntent(), intent);
        intent.putExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS, params);
        getTransitionHelper().startActivityForResultWithTransition(
                this, intent, FINANCED_DEVICE_PREPARE_REQUEST_CODE);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        if (v instanceof TextView) {
            mContextMenuMaker.populateMenuContent(menu, (TextView) v);
        }
    }

    @Override
    public void onBackPressed() {
        if (mUtils.isOrganizationOwnedAllowed(mController.getParams())) {
            showDialog(mUtils.createCancelProvisioningResetDialogBuilder(),
                    BACK_PRESSED_DIALOG_RESET);
        } else {
            showDialog(mUtils.createCancelProvisioningDialogBuilder(),
                    BACK_PRESSED_DIALOG_CLOSE_ACTIVITY);
        }
    }

    private void logMetrics() {
        final ProvisioningAnalyticsTracker analyticsTracker =
                new ProvisioningAnalyticsTracker(
                        MetricsWriterFactory.getMetricsWriter(this, new SettingsFacade()),
                        new ManagedProvisioningSharedPreferences(this));
        int nightMode = getResources().getConfiguration().uiMode & UI_MODE_NIGHT_MASK;
        analyticsTracker.logIsNightMode(nightMode == UI_MODE_NIGHT_YES);
    }

    /**
     * Constructs {@link PreProvisioningActivityController} for a given {@link
     * PreProvisioningActivity}
     */
    interface ControllerProvider {
        /**
         * Constructs {@link PreProvisioningActivityController} for a given {@link
         * PreProvisioningActivity}
         */
        PreProvisioningActivityController getInstance(PreProvisioningActivity activity);
    }
}
