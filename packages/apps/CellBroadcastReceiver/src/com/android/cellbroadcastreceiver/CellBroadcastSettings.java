/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.cellbroadcastreceiver;

import android.annotation.NonNull;
import android.app.ActionBar;
import android.app.ActivityManager;
import android.app.Fragment;
import android.app.backup.BackupManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.os.UserManager;
import android.os.Vibrator;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.MenuItem;
import android.widget.Switch;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragment;
import androidx.preference.PreferenceManager;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;

import com.android.internal.annotations.VisibleForTesting;
import com.android.modules.utils.build.SdkLevel;
import com.android.settingslib.collapsingtoolbar.CollapsingToolbarBaseActivity;
import com.android.settingslib.widget.MainSwitchPreference;
import com.android.settingslib.widget.OnMainSwitchChangeListener;

import java.util.HashMap;
import java.util.Map;

/**
 * Settings activity for the cell broadcast receiver.
 */
public class CellBroadcastSettings extends CollapsingToolbarBaseActivity {

    private static final String TAG = "CellBroadcastSettings";

    private static final boolean DBG = false;

    /**
     * Keys for user preferences.
     * When adding a new preference, make sure to clear its value in resetAllPreferences.
     */
    // Preference key for alert header (A text view, not clickable).
    public static final String KEY_ALERTS_HEADER = "alerts_header";

    // Preference key for a main toggle to enable/disable all alerts message (default enabled).
    public static final String KEY_ENABLE_ALERTS_MASTER_TOGGLE = "enable_alerts_master_toggle";

    // Preference key for whether to enable public safety messages (default enabled).
    public static final String KEY_ENABLE_PUBLIC_SAFETY_MESSAGES = "enable_public_safety_messages";

    // Preference key for whether to show full-screen public safety message (pop-up dialog), If set
    // to false, only display from message history and sms inbox if enabled. A foreground
    // notification might also be shown if enabled.
    public static final String KEY_ENABLE_PUBLIC_SAFETY_MESSAGES_FULL_SCREEN =
            "enable_public_safety_messages_full_screen";

    // Preference key for whether to enable emergency alerts (default enabled).
    public static final String KEY_ENABLE_EMERGENCY_ALERTS = "enable_emergency_alerts";

    // Enable vibration on alert (unless main volume is silent).
    public static final String KEY_ENABLE_ALERT_VIBRATE = "enable_alert_vibrate";

    // Speak contents of alert after playing the alert sound.
    public static final String KEY_ENABLE_ALERT_SPEECH = "enable_alert_speech";

    // Play alert sound in full volume regardless Do Not Disturb is on.
    public static final String KEY_OVERRIDE_DND = "override_dnd";

    public static final String KEY_OVERRIDE_DND_SETTINGS_CHANGED =
            "override_dnd_settings_changed";

    // Preference category for emergency alert and CMAS settings.
    public static final String KEY_CATEGORY_EMERGENCY_ALERTS = "category_emergency_alerts";

    // Preference category for alert preferences.
    public static final String KEY_CATEGORY_ALERT_PREFERENCES = "category_alert_preferences";

    // Show checkbox for Presidential alerts in settings
    // Whether to display CMAS presidential alert notifications (always enabled).
    public static final String KEY_ENABLE_CMAS_PRESIDENTIAL_ALERTS =
            "enable_cmas_presidential_alerts";

    // Whether to display CMAS extreme threat notifications (default is enabled).
    public static final String KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS =
            "enable_cmas_extreme_threat_alerts";

    // Whether to display CMAS severe threat notifications (default is enabled).
    public static final String KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS =
            "enable_cmas_severe_threat_alerts";

    // Whether to display CMAS amber alert messages (default is enabled).
    public static final String KEY_ENABLE_CMAS_AMBER_ALERTS = "enable_cmas_amber_alerts";

    // Whether to display monthly test messages (default is disabled).
    public static final String KEY_ENABLE_TEST_ALERTS = "enable_test_alerts";

    // Whether to display exercise test alerts.
    public static final String KEY_ENABLE_EXERCISE_ALERTS = "enable_exercise_alerts";

    // Whether to display operator defined test alerts
    public static final String KEY_OPERATOR_DEFINED_ALERTS = "enable_operator_defined_alerts";

    // Whether to display state/local test messages (default disabled).
    public static final String KEY_ENABLE_STATE_LOCAL_TEST_ALERTS =
            "enable_state_local_test_alerts";

    // Preference key for whether to enable area update information notifications
    // Enabled by default for phones sold in Brazil and India, otherwise this setting may be hidden.
    public static final String KEY_ENABLE_AREA_UPDATE_INFO_ALERTS =
            "enable_area_update_info_alerts";

    // Preference key for initial opt-in/opt-out dialog.
    public static final String KEY_SHOW_CMAS_OPT_OUT_DIALOG = "show_cmas_opt_out_dialog";

    // Alert reminder interval ("once" = single 2 minute reminder).
    public static final String KEY_ALERT_REMINDER_INTERVAL = "alert_reminder_interval";

    // Preference key for emergency alerts history
    public static final String KEY_EMERGENCY_ALERT_HISTORY = "emergency_alert_history";

    // For watch layout
    private static final String KEY_WATCH_ALERT_REMINDER = "watch_alert_reminder";

    // Whether to receive alert in second language code
    public static final String KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE =
            "receive_cmas_in_second_language";

    /* End of user preferences keys section. */

    // Resource cache
    private static final Map<Integer, Resources> sResourcesCache = new HashMap<>();

    // Intent sent from cellbroadcastreceiver to notify cellbroadcastservice that area info update
    // is disabled/enabled.
    private static final String AREA_INFO_UPDATE_ACTION =
            "com.android.cellbroadcastreceiver.action.AREA_UPDATE_INFO_ENABLED";
    private static final String AREA_INFO_UPDATE_ENABLED_EXTRA = "enable";

    /**
     * This permission is only granted to the cellbroadcast mainline module and thus can be
     * used for permission check within CBR and CBS.
     */
    private static final String CBR_MODULE_PERMISSION =
            "com.android.cellbroadcastservice.FULL_ACCESS_CELL_BROADCAST_HISTORY";

    // Key for shared preference which represents whether user has changed any preference
    private static final String ANY_PREFERENCE_CHANGED_BY_USER = "any_preference_changed_by_user";

    // Test override for disabling the subId specific resources
    private static boolean sUseResourcesForSubId = true;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // for backward compatibility on R devices
        if (!SdkLevel.isAtLeastS()) {
            setCustomizeContentView(R.layout.cell_broadcast_list_collapsing_no_toobar);
        }
        super.onCreate(savedInstanceState);

        // for backward compatibility on R devices
        if (!SdkLevel.isAtLeastS()) {
            ActionBar actionBar = getActionBar();
            if (actionBar != null) {
                // android.R.id.home will be triggered in onOptionsItemSelected()
                actionBar.setDisplayHomeAsUpEnabled(true);
            }
        }

        UserManager userManager = (UserManager) getSystemService(Context.USER_SERVICE);
        if (userManager.hasUserRestriction(UserManager.DISALLOW_CONFIG_CELL_BROADCASTS)) {
            setContentView(R.layout.cell_broadcast_disallowed_preference_screen);
            return;
        }

        // We only add new CellBroadcastSettingsFragment if no fragment is restored.
        Fragment fragment = getFragmentManager().findFragmentById(
                com.android.settingslib.collapsingtoolbar.R.id.content_frame);
        if (fragment == null) {
            fragment = new CellBroadcastSettingsFragment();
            getFragmentManager()
                    .beginTransaction()
                    .add(com.android.settingslib.collapsingtoolbar.R.id.content_frame, fragment)
                    .commit();
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        getWindow().addSystemFlags(
                android.view.WindowManager.LayoutParams
                        .SYSTEM_FLAG_HIDE_NON_SYSTEM_OVERLAY_WINDOWS);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            // Respond to the action bar's Up/Home button
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * Reset all user values for preferences (stored in shared preferences).
     *
     * @param c the application context
     */
    public static void resetAllPreferences(Context c) {
        SharedPreferences.Editor e = PreferenceManager.getDefaultSharedPreferences(c).edit();
        e.remove(KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS)
                .remove(KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS)
                .remove(KEY_ENABLE_CMAS_AMBER_ALERTS)
                .remove(KEY_ENABLE_PUBLIC_SAFETY_MESSAGES)
                .remove(KEY_ENABLE_PUBLIC_SAFETY_MESSAGES_FULL_SCREEN)
                .remove(KEY_ENABLE_EMERGENCY_ALERTS)
                .remove(KEY_ALERT_REMINDER_INTERVAL)
                .remove(KEY_ENABLE_ALERT_SPEECH)
                .remove(KEY_OVERRIDE_DND)
                .remove(KEY_ENABLE_AREA_UPDATE_INFO_ALERTS)
                .remove(KEY_ENABLE_TEST_ALERTS)
                .remove(KEY_ENABLE_STATE_LOCAL_TEST_ALERTS)
                .remove(KEY_ENABLE_ALERT_VIBRATE)
                .remove(KEY_ENABLE_CMAS_PRESIDENTIAL_ALERTS)
                .remove(KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE)
                .remove(KEY_ENABLE_EXERCISE_ALERTS)
                .remove(KEY_OPERATOR_DEFINED_ALERTS);
        // If the device is in test harness mode, reset main toggle should only happen on the
        // first boot.
        if (!ActivityManager.isRunningInUserTestHarness()) {
          Log.d(TAG, "In not test harness mode. reset main toggle.");
          e.remove(KEY_ENABLE_ALERTS_MASTER_TOGGLE);
        }
        PackageManager pm = c.getPackageManager();
        if (pm.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            e.remove(KEY_WATCH_ALERT_REMINDER);
        }
        e.commit();

        if (pm.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            PreferenceManager.setDefaultValues(c, R.xml.watch_preferences, true);
        } else {
            PreferenceManager.setDefaultValues(c, R.xml.preferences, true);
        }
        setPreferenceChanged(c, false);
    }

    /**
     * Return true if user has modified any preference manually.
     * @param c the application context
     * @return
     */
    public static boolean hasAnyPreferenceChanged(Context c) {
        return PreferenceManager.getDefaultSharedPreferences(c)
                .getBoolean(ANY_PREFERENCE_CHANGED_BY_USER, false);
    }

    private static void setPreferenceChanged(Context c, boolean changed) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        sp.edit().putBoolean(ANY_PREFERENCE_CHANGED_BY_USER, changed).apply();
    }

    /**
     * New fragment-style implementation of preferences.
     */
    public static class CellBroadcastSettingsFragment extends PreferenceFragment {

        private TwoStatePreference mExtremeCheckBox;
        private TwoStatePreference mSevereCheckBox;
        private TwoStatePreference mAmberCheckBox;
        private MainSwitchPreference mMasterToggle;
        private TwoStatePreference mPublicSafetyMessagesChannelCheckBox;
        private TwoStatePreference mPublicSafetyMessagesChannelFullScreenCheckBox;
        private TwoStatePreference mEmergencyAlertsCheckBox;
        private ListPreference mReminderInterval;
        private TwoStatePreference mSpeechCheckBox;
        private TwoStatePreference mOverrideDndCheckBox;
        private TwoStatePreference mAreaUpdateInfoCheckBox;
        private TwoStatePreference mTestCheckBox;
        private TwoStatePreference mExerciseTestCheckBox;
        private TwoStatePreference mOperatorDefinedCheckBox;
        private TwoStatePreference mStateLocalTestCheckBox;
        private TwoStatePreference mEnableVibrateCheckBox;
        private Preference mAlertHistory;
        private Preference mAlertsHeader;
        private PreferenceCategory mAlertCategory;
        private PreferenceCategory mAlertPreferencesCategory;
        private boolean mDisableSevereWhenExtremeDisabled = true;

        // WATCH
        private TwoStatePreference mAlertReminder;

        // Show checkbox for Presidential alerts in settings
        private TwoStatePreference mPresidentialCheckBox;

        // on/off switch in settings for receiving alert in second language code
        private TwoStatePreference mReceiveCmasInSecondLanguageCheckBox;

        private final BroadcastReceiver mTestingModeChangedReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                switch (intent.getAction()) {
                    case CellBroadcastReceiver.ACTION_TESTING_MODE_CHANGED:
                        updatePreferenceVisibility();
                        break;
                }
            }
        };

        private void initPreferences() {
            mExtremeCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS);
            mSevereCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS);
            mAmberCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_CMAS_AMBER_ALERTS);
            mMasterToggle = (MainSwitchPreference)
                    findPreference(KEY_ENABLE_ALERTS_MASTER_TOGGLE);
            mPublicSafetyMessagesChannelCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_PUBLIC_SAFETY_MESSAGES);
            mPublicSafetyMessagesChannelFullScreenCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_PUBLIC_SAFETY_MESSAGES_FULL_SCREEN);
            mEmergencyAlertsCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_EMERGENCY_ALERTS);
            mReminderInterval = (ListPreference)
                    findPreference(KEY_ALERT_REMINDER_INTERVAL);
            mSpeechCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_ALERT_SPEECH);
            mOverrideDndCheckBox = (TwoStatePreference)
                    findPreference(KEY_OVERRIDE_DND);
            mAreaUpdateInfoCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_AREA_UPDATE_INFO_ALERTS);
            mTestCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_TEST_ALERTS);
            mExerciseTestCheckBox = (TwoStatePreference) findPreference(KEY_ENABLE_EXERCISE_ALERTS);
            mOperatorDefinedCheckBox = (TwoStatePreference)
                    findPreference(KEY_OPERATOR_DEFINED_ALERTS);
            mStateLocalTestCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_STATE_LOCAL_TEST_ALERTS);
            mAlertHistory = findPreference(KEY_EMERGENCY_ALERT_HISTORY);
            mAlertsHeader = findPreference(KEY_ALERTS_HEADER);
            mReceiveCmasInSecondLanguageCheckBox = (TwoStatePreference) findPreference
                    (KEY_RECEIVE_CMAS_IN_SECOND_LANGUAGE);
            mEnableVibrateCheckBox = findPreference(KEY_ENABLE_ALERT_VIBRATE);

            // Show checkbox for Presidential alerts in settings
            mPresidentialCheckBox = (TwoStatePreference)
                    findPreference(KEY_ENABLE_CMAS_PRESIDENTIAL_ALERTS);

            PackageManager pm = getActivity().getPackageManager();
            if (pm.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
                mAlertReminder = (TwoStatePreference)
                        findPreference(KEY_WATCH_ALERT_REMINDER);
                if (Integer.valueOf(mReminderInterval.getValue()) == 0) {
                    mAlertReminder.setChecked(false);
                } else {
                    mAlertReminder.setChecked(true);
                }
                mAlertReminder.setOnPreferenceChangeListener((p, newVal) -> {
                    try {
                        mReminderInterval.setValueIndex((Boolean) newVal ? 1 : 3);
                    } catch (IndexOutOfBoundsException e) {
                        mReminderInterval.setValue(String.valueOf(0));
                        Log.w(TAG, "Setting default value");
                    }
                    return true;
                });
                PreferenceScreen watchScreen = (PreferenceScreen)
                        findPreference(KEY_CATEGORY_ALERT_PREFERENCES);
                watchScreen.removePreference(mReminderInterval);
            } else {
                mAlertPreferencesCategory = (PreferenceCategory)
                        findPreference(KEY_CATEGORY_ALERT_PREFERENCES);
                mAlertCategory = (PreferenceCategory)
                        findPreference(KEY_CATEGORY_EMERGENCY_ALERTS);
            }
        }

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {

            LocalBroadcastManager.getInstance(getContext())
                    .registerReceiver(mTestingModeChangedReceiver, new IntentFilter(
                            CellBroadcastReceiver.ACTION_TESTING_MODE_CHANGED));

            // Load the preferences from an XML resource
            PackageManager pm = getActivity().getPackageManager();
            if (pm.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
                addPreferencesFromResource(R.xml.watch_preferences);
            } else {
                addPreferencesFromResource(R.xml.preferences);
            }

            initPreferences();

            Resources res = CellBroadcastSettings.getResourcesForDefaultSubId(getContext());

            mDisableSevereWhenExtremeDisabled = res.getBoolean(
                    R.bool.disable_severe_when_extreme_disabled);

            final OnMainSwitchChangeListener mainSwitchListener = new OnMainSwitchChangeListener() {
                @Override
                public void onSwitchChanged(Switch switchView, boolean isChecked) {
                    setAlertsEnabled(isChecked);
                }
            };

            // Handler for settings that require us to reconfigure enabled channels in radio
            Preference.OnPreferenceChangeListener startConfigServiceListener =
                    new Preference.OnPreferenceChangeListener() {
                        @Override
                        public boolean onPreferenceChange(Preference pref, Object newValue) {
                            CellBroadcastReceiver.startConfigService(pref.getContext(),
                                    CellBroadcastConfigService.ACTION_ENABLE_CHANNELS);
                            setPreferenceChanged(getContext(), true);

                            if (mDisableSevereWhenExtremeDisabled) {
                                if (pref.getKey().equals(KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS)) {
                                    boolean isExtremeAlertChecked = (Boolean) newValue;
                                    if (mSevereCheckBox != null) {
                                        mSevereCheckBox.setEnabled(isExtremeAlertChecked);
                                        mSevereCheckBox.setChecked(false);
                                    }
                                }
                            }

                            // check if area update was disabled
                            if (pref.getKey().equals(KEY_ENABLE_AREA_UPDATE_INFO_ALERTS)) {
                                boolean isEnabledAlert = (Boolean) newValue;
                                notifyAreaInfoUpdate(isEnabledAlert);
                            }

                            // Notify backup manager a backup pass is needed.
                            new BackupManager(getContext()).dataChanged();
                            return true;
                        }
                    };

            initReminderIntervalList();

            if (mMasterToggle != null) {
                mMasterToggle.addOnSwitchChangeListener(mainSwitchListener);
                // If allow alerts are disabled, we turn all sub-alerts off. If it's enabled, we
                // leave them as they are.
                if (!mMasterToggle.isChecked()) {
                    setAlertsEnabled(false);
                }
            }
            // note that mPresidentialCheckBox does not use the startConfigServiceListener because
            // the user is never allowed to change the preference
            if (mAreaUpdateInfoCheckBox != null) {
                mAreaUpdateInfoCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mExtremeCheckBox != null) {
                mExtremeCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mPublicSafetyMessagesChannelCheckBox != null) {
                mPublicSafetyMessagesChannelCheckBox.setOnPreferenceChangeListener(
                        startConfigServiceListener);
            }
            if (mPublicSafetyMessagesChannelFullScreenCheckBox != null) {
                mPublicSafetyMessagesChannelFullScreenCheckBox.setOnPreferenceChangeListener(
                        startConfigServiceListener);
            }
            if (mEmergencyAlertsCheckBox != null) {
                mEmergencyAlertsCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mSevereCheckBox != null) {
                mSevereCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
                if (mDisableSevereWhenExtremeDisabled) {
                    if (mExtremeCheckBox != null) {
                        mSevereCheckBox.setEnabled(mExtremeCheckBox.isChecked());
                    }
                }
            }
            if (mAmberCheckBox != null) {
                mAmberCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mTestCheckBox != null) {
                mTestCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mExerciseTestCheckBox != null) {
                mExerciseTestCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mOperatorDefinedCheckBox != null) {
                mOperatorDefinedCheckBox.setOnPreferenceChangeListener(startConfigServiceListener);
            }
            if (mStateLocalTestCheckBox != null) {
                mStateLocalTestCheckBox.setOnPreferenceChangeListener(
                        startConfigServiceListener);
            }

            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());

            if (mOverrideDndCheckBox != null) {
                if (!sp.getBoolean(KEY_OVERRIDE_DND_SETTINGS_CHANGED, false)) {
                    // If the user hasn't changed this settings yet, use the default settings
                    // from resource overlay.
                    mOverrideDndCheckBox.setChecked(res.getBoolean(R.bool.override_dnd_default));
                }
                mOverrideDndCheckBox.setOnPreferenceChangeListener(
                        (pref, newValue) -> {
                            sp.edit().putBoolean(KEY_OVERRIDE_DND_SETTINGS_CHANGED,
                                    true).apply();
                            updateVibrationPreference((boolean) newValue);
                            return true;
                        });
            }

            if (mAlertHistory != null) {
                mAlertHistory.setOnPreferenceClickListener(
                        preference -> {
                            final Intent intent = new Intent(getContext(),
                                    CellBroadcastListActivity.class);
                            startActivity(intent);
                            return true;
                        });
            }

            updateVibrationPreference(sp.getBoolean(CellBroadcastSettings.KEY_OVERRIDE_DND,
                    false));
            updatePreferenceVisibility();
        }

        /**
         * Update the vibration preference based on override DND. If DND is overridden, then do
         * not allow users to turn off vibration.
         *
         * @param overrideDnd {@code true} if the alert will be played at full volume, regardless
         * DND settings.
         */
        private void updateVibrationPreference(boolean overrideDnd) {
            if (mEnableVibrateCheckBox != null) {
                if (overrideDnd) {
                    // If DND is enabled, always enable vibration.
                    mEnableVibrateCheckBox.setChecked(true);
                }
                // Grey out the preference if DND is overridden.
                mEnableVibrateCheckBox.setEnabled(!overrideDnd);
            }
        }

        /**
         * Dynamically update each preference's visibility based on configuration.
         */
        private void updatePreferenceVisibility() {
            Resources res = CellBroadcastSettings.getResourcesForDefaultSubId(getContext());

            CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(
                    getContext(), SubscriptionManager.getDefaultSubscriptionId());

            if (mMasterToggle != null) {
                mMasterToggle.setVisible(res.getBoolean(R.bool.show_main_switch_settings));
            }

            if (mPresidentialCheckBox != null) {
                mPresidentialCheckBox.setVisible(
                        res.getBoolean(R.bool.show_presidential_alerts_settings));
            }

            if (mExtremeCheckBox != null) {
                mExtremeCheckBox.setVisible(res.getBoolean(R.bool.show_extreme_alert_settings)
                        && !channelManager.getCellBroadcastChannelRanges(
                                R.array.cmas_alert_extreme_channels_range_strings).isEmpty());
            }

            if (mSevereCheckBox != null) {
                mSevereCheckBox.setVisible(res.getBoolean(R.bool.show_severe_alert_settings)
                        && !channelManager.getCellBroadcastChannelRanges(
                                R.array.cmas_alerts_severe_range_strings).isEmpty());
            }

            if (mAmberCheckBox != null) {
                mAmberCheckBox.setVisible(res.getBoolean(R.bool.show_amber_alert_settings)
                        && !channelManager.getCellBroadcastChannelRanges(
                                R.array.cmas_amber_alerts_channels_range_strings).isEmpty());
            }

            if (mPublicSafetyMessagesChannelCheckBox != null) {
                mPublicSafetyMessagesChannelCheckBox.setVisible(
                        res.getBoolean(R.bool.show_public_safety_settings)
                                && !channelManager.getCellBroadcastChannelRanges(
                                        R.array.public_safety_messages_channels_range_strings)
                                .isEmpty());
            }
            // this is the matching full screen settings for public safety toggle. shown only if
            // public safety toggle is displayed.
            if (mPublicSafetyMessagesChannelFullScreenCheckBox != null) {
                mPublicSafetyMessagesChannelFullScreenCheckBox.setVisible(
                        res.getBoolean(R.bool.show_public_safety_full_screen_settings)
                                && (mPublicSafetyMessagesChannelCheckBox != null
                                && mPublicSafetyMessagesChannelCheckBox.isVisible()));
            }

            if (mTestCheckBox != null) {
                mTestCheckBox.setVisible(isTestAlertsToggleVisible(getContext()));
            }

            if (mExerciseTestCheckBox != null) {
                boolean visible = false;
                if (res.getBoolean(R.bool.show_separate_exercise_settings)) {
                    if (res.getBoolean(R.bool.show_exercise_settings)
                            || CellBroadcastReceiver.isTestingMode(getContext())) {
                        if (!channelManager.getCellBroadcastChannelRanges(
                                R.array.exercise_alert_range_strings).isEmpty()) {
                            visible = true;
                        }
                    }
                }
                mExerciseTestCheckBox.setVisible(visible);
            }

            if (mOperatorDefinedCheckBox != null) {
                boolean visible = false;
                if (res.getBoolean(R.bool.show_separate_operator_defined_settings)) {
                    if (res.getBoolean(R.bool.show_operator_defined_settings)
                            || CellBroadcastReceiver.isTestingMode(getContext())) {
                        if (!channelManager.getCellBroadcastChannelRanges(
                                R.array.operator_defined_alert_range_strings).isEmpty()) {
                            visible = true;
                        }
                    }
                }
                mOperatorDefinedCheckBox.setVisible(visible);
            }

            if (mEmergencyAlertsCheckBox != null) {
                mEmergencyAlertsCheckBox.setVisible(!channelManager.getCellBroadcastChannelRanges(
                        R.array.emergency_alerts_channels_range_strings).isEmpty());
            }

            if (mStateLocalTestCheckBox != null) {
                mStateLocalTestCheckBox.setVisible(
                        res.getBoolean(R.bool.show_state_local_test_settings)
                                && !channelManager.getCellBroadcastChannelRanges(
                                        R.array.state_local_test_alert_range_strings).isEmpty());
            }

            if (mReceiveCmasInSecondLanguageCheckBox != null) {
                mReceiveCmasInSecondLanguageCheckBox.setVisible(!res.getString(
                        R.string.emergency_alert_second_language_code).isEmpty());
            }

            if (mAreaUpdateInfoCheckBox != null) {
                mAreaUpdateInfoCheckBox.setVisible(
                        res.getBoolean(R.bool.config_showAreaUpdateInfoSettings));
            }

            if (mOverrideDndCheckBox != null) {
                mOverrideDndCheckBox.setVisible(res.getBoolean(R.bool.show_override_dnd_settings));
            }

            if (mEnableVibrateCheckBox != null) {
                // Only show vibrate toggle when override DND toggle is available to users, or when
                // override DND default is turned off.
                // In some countries, override DND is always on, which means vibration is always on.
                // In that case, no need to show vibration toggle for users.
                Vibrator vibrator = getContext().getSystemService(Vibrator.class);
                boolean supportVibration = (vibrator != null) && vibrator.hasVibrator();
                mEnableVibrateCheckBox.setVisible(supportVibration
                        && (res.getBoolean(R.bool.show_override_dnd_settings) ||
                        !res.getBoolean(R.bool.override_dnd)));
            }
            if (mAlertsHeader != null) {
                mAlertsHeader.setVisible(
                        !getContext().getString(R.string.alerts_header_summary).isEmpty());
            }

            if (mSpeechCheckBox != null) {
                mSpeechCheckBox.setVisible(res.getBoolean(R.bool.show_alert_speech_setting)
                        || getActivity().getPackageManager()
                        .hasSystemFeature(PackageManager.FEATURE_WATCH));
            }
        }

        private void initReminderIntervalList() {
            Resources res = CellBroadcastSettings.getResourcesForDefaultSubId(getContext());

            String[] activeValues =
                    res.getStringArray(R.array.alert_reminder_interval_active_values);
            String[] allEntries = res.getStringArray(R.array.alert_reminder_interval_entries);
            String[] newEntries = new String[activeValues.length];

            // Only add active interval to the list
            for (int i = 0; i < activeValues.length; i++) {
                int index = mReminderInterval.findIndexOfValue(activeValues[i]);
                if (index != -1) {
                    newEntries[i] = allEntries[index];
                    if (DBG) Log.d(TAG, "Added " + allEntries[index]);
                } else {
                    Log.e(TAG, "Can't find " + activeValues[i]);
                }
            }

            mReminderInterval.setEntries(newEntries);
            mReminderInterval.setEntryValues(activeValues);
            mReminderInterval.setSummary(mReminderInterval.getEntry());
            mReminderInterval.setOnPreferenceChangeListener(
                    new Preference.OnPreferenceChangeListener() {
                        @Override
                        public boolean onPreferenceChange(Preference pref, Object newValue) {
                            final ListPreference listPref = (ListPreference) pref;
                            final int idx = listPref.findIndexOfValue((String) newValue);
                            listPref.setSummary(listPref.getEntries()[idx]);
                            return true;
                        }
                    });
        }


        private void setAlertsEnabled(boolean alertsEnabled) {
            if (mSevereCheckBox != null) {
                mSevereCheckBox.setEnabled(alertsEnabled);
                mSevereCheckBox.setChecked(alertsEnabled);
            }
            if (mExtremeCheckBox != null) {
                mExtremeCheckBox.setEnabled(alertsEnabled);
                mExtremeCheckBox.setChecked(alertsEnabled);
            }
            if (mAmberCheckBox != null) {
                mAmberCheckBox.setEnabled(alertsEnabled);
                mAmberCheckBox.setChecked(alertsEnabled);
            }
            if (mAreaUpdateInfoCheckBox != null) {
                mAreaUpdateInfoCheckBox.setEnabled(alertsEnabled);
                mAreaUpdateInfoCheckBox.setChecked(alertsEnabled);
                notifyAreaInfoUpdate(alertsEnabled);
            }
            if (mEmergencyAlertsCheckBox != null) {
                mEmergencyAlertsCheckBox.setEnabled(alertsEnabled);
                mEmergencyAlertsCheckBox.setChecked(alertsEnabled);
            }
            if (mPublicSafetyMessagesChannelCheckBox != null) {
                mPublicSafetyMessagesChannelCheckBox.setEnabled(alertsEnabled);
                mPublicSafetyMessagesChannelCheckBox.setChecked(alertsEnabled);
            }
            if (mStateLocalTestCheckBox != null) {
                mStateLocalTestCheckBox.setEnabled(alertsEnabled);
                mStateLocalTestCheckBox.setChecked(alertsEnabled);
            }
            if (mTestCheckBox != null) {
                mTestCheckBox.setEnabled(alertsEnabled);
                mTestCheckBox.setChecked(alertsEnabled);
            }
            if (mExerciseTestCheckBox != null) {
                mExerciseTestCheckBox.setEnabled(alertsEnabled);
                mExerciseTestCheckBox.setChecked(alertsEnabled);
            }
            if (mOperatorDefinedCheckBox != null) {
                mOperatorDefinedCheckBox.setEnabled(alertsEnabled);
                mOperatorDefinedCheckBox.setChecked(alertsEnabled);
            }
        }

        private void notifyAreaInfoUpdate(boolean enabled) {
            Intent areaInfoIntent = new Intent(AREA_INFO_UPDATE_ACTION);
            areaInfoIntent.putExtra(AREA_INFO_UPDATE_ENABLED_EXTRA, enabled);
            // sending broadcast protected by the permission which is only
            // granted for CBR mainline module.
            getContext().sendBroadcast(areaInfoIntent, CBR_MODULE_PERMISSION);
        }


        @Override
        public void onResume() {
            super.onResume();
            updatePreferenceVisibility();
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            LocalBroadcastManager.getInstance(getContext())
                    .unregisterReceiver(mTestingModeChangedReceiver);
        }
    }

    public static boolean isTestAlertsToggleVisible(Context context) {
        CellBroadcastChannelManager channelManager = new CellBroadcastChannelManager(context,
                SubscriptionManager.getDefaultSubscriptionId());
        Resources res = CellBroadcastSettings.getResourcesForDefaultSubId(context);
        boolean isTestAlertsAvailable = !channelManager.getCellBroadcastChannelRanges(
                R.array.required_monthly_test_range_strings).isEmpty()
                || (!channelManager.getCellBroadcastChannelRanges(
                R.array.exercise_alert_range_strings).isEmpty()
                /** exercise toggle is controlled under the main test toggle */
                && (!res.getBoolean(R.bool.show_separate_exercise_settings)))
                || (!channelManager.getCellBroadcastChannelRanges(
                R.array.operator_defined_alert_range_strings).isEmpty()
                /** operator defined toggle is controlled under the main test toggle */
                && (!res.getBoolean(R.bool.show_separate_operator_defined_settings)))
                || !channelManager.getCellBroadcastChannelRanges(
                R.array.etws_test_alerts_range_strings).isEmpty();

        return (res.getBoolean(R.bool.show_test_settings)
                || CellBroadcastReceiver.isTestingMode(context))
                && isTestAlertsAvailable;
    }

    public static boolean isFeatureEnabled(Context context, String feature, boolean defaultValue) {
        CarrierConfigManager configManager =
                (CarrierConfigManager) context.getSystemService(Context.CARRIER_CONFIG_SERVICE);

        if (configManager != null) {
            PersistableBundle carrierConfig = configManager.getConfig();
            if (carrierConfig != null) {
                return carrierConfig.getBoolean(feature, defaultValue);
            }
        }

        return defaultValue;
    }

    /**
     * Override used by tests so that we don't call
     * SubscriptionManager.getResourcesForSubId, which is a static unmockable
     * method.
     */
    @VisibleForTesting
    public static void setUseResourcesForSubId(boolean useResourcesForSubId) {
        sUseResourcesForSubId = useResourcesForSubId;
    }

    /**
     * Get the device resource based on SIM
     *
     * @param context Context
     * @param subId Subscription index
     *
     * @return The resource
     */
    public static @NonNull Resources getResources(@NonNull Context context, int subId) {
        // based on the latest design, subId can be valid earlier than mcc mnc is known to telephony
        // check if sim is loaded to avoid caching the wrong resources.
        TelephonyManager tm = context.getSystemService(TelephonyManager.class);
        boolean isSimLoaded = tm.getSimApplicationState(SubscriptionManager.getSlotIndex(subId))
                == TelephonyManager.SIM_STATE_LOADED;
        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID
                || !SubscriptionManager.isValidSubscriptionId(subId) || !sUseResourcesForSubId
                || !isSimLoaded) {
            return context.getResources();
        }

        if (sResourcesCache.containsKey(subId)) {
            return sResourcesCache.get(subId);
        }

        Resources res = SubscriptionManager.getResourcesForSubId(context, subId);
        sResourcesCache.put(subId, res);

        return res;
    }

    /**
     * Get the resources using the default subscription ID.
     * @param context Context
     * @return the Resources for the default subscription ID, or if there is no default subscription
     * from SubscriptionManager, the resources for the latest loaded SIM.
     */
    public static @NonNull Resources getResourcesForDefaultSubId(@NonNull Context context) {
        return getResources(context, SubscriptionManager.getDefaultSubscriptionId());
    }
}
