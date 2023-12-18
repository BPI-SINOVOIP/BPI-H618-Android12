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

package android.platform.helpers;

import java.util.HashMap;
import java.util.Map;

/** Configuration for Settings Application */
public class AutoSettingsConfigUtility implements IAutoConfigUtility {
    private static final String LOG_TAG = AutoSettingsConfigUtility.class.getSimpleName();

    // Settings Config
    private static final String SETTINGS_TITLE_TEXT = "Settings";
    private static final String SETTING_APP_PACKAGE = "com.android.car.settings";
    private static final String SETTING_RRO_PACKAGE = "com.android.car.settings.googlecarui.rro";
    private static final String SETTING_INTELLIGENCE_PACKAGE = "com.android.settings.intelligence";
    private static final String PERMISSIONS_PACKAGE = "com.android.permissioncontroller";
    private static final String OPEN_SETTINGS_COMMAND = "am start -a android.settings.SETTINGS";
    private static final String OPEN_QUICK_SETTINGS_COMMAND =
            "am start -n "
                    + "com.android.car.settings/"
                    + "com.android.car.settings.common.CarSettingActivities$QuickSettingActivity";

    // Config Maps
    private Map<String, String[]> mSettingsOptionsMap;
    private Map<String, String[]> mSettingsPathMap;
    private Map<String, AutoConfiguration> mSettingsConfigMap;

    private static AutoSettingsConfigUtility sAutoSettingsConfigInstance = null;

    private AutoSettingsConfigUtility() {
        // Initialize Settings Config Maps
        mSettingsOptionsMap = new HashMap<String, String[]>();
        mSettingsPathMap = new HashMap<String, String[]>();
        mSettingsConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get instance of Auto Settings Utility */
    public static IAutoConfigUtility getInstance() {
        if (sAutoSettingsConfigInstance == null) {
            sAutoSettingsConfigInstance = new AutoSettingsConfigUtility();
        }
        return sAutoSettingsConfigInstance;
    }

    /**
     * Get Path for given Setting
     *
     * @param menu Setting Name
     */
    public String[] getPath(String menu) {
        String[] settingPath = mSettingsPathMap.get(menu);
        if (settingPath == null) {
            settingPath = new String[] {menu};
        }
        return settingPath;
    }

    /**
     * Add path for given Setting
     *
     * @param menu Setting Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        mSettingsPathMap.put(menu, path);
    }

    /**
     * Get available menu options for given Setting
     *
     * @param menu Setting Name
     */
    public String[] getAvailableOptions(String menu) {
        return mSettingsOptionsMap.get(menu);
    }

    /**
     * Add available menu options for given Setting
     *
     * @param menu Setting Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        mSettingsOptionsMap.put(menu, options);
    }

    /**
     * Get Setting Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mSettingsConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mSettingsConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate Setting Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mSettingsConfigMap.get(configName) != null);
    }

    /**
     * Validate Setting Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mSettingsConfigMap.get(configName) == null) {
            return false;
        }
        return (mSettingsConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Setting Options
     *
     * @param menu Setting Name
     */
    public boolean isValidOption(String menu) {
        return (mSettingsOptionsMap.get(menu) != null);
    }

    /**
     * Validate Setting Path
     *
     * @param menu Setting Name
     */
    public boolean isValidPath(String menu) {
        return (mSettingsPathMap.get(menu) != null);
    }

    /** Load default configuration for Settings application */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default Settings Application Config
        loadDefaultSettingsAppConfig(mApplicationConfigMap);

        // Default Setting Options
        loadDefaultSettingOptions(mSettingsOptionsMap);

        // Default Setting Paths
        loadDefaultSettingPaths(mSettingsPathMap);

        // Default Setting Resource Config
        loadDefaultSettingResourceConfig(mSettingsConfigMap);
    }

    private void loadDefaultSettingsAppConfig(Map<String, String> mApplicationConfigMap) {
        // Add default settings title text
        mApplicationConfigMap.put(AutoConfigConstants.SETTINGS_TITLE_TEXT, SETTINGS_TITLE_TEXT);
        // Add default settings package
        mApplicationConfigMap.put(AutoConfigConstants.SETTINGS_PACKAGE, SETTING_APP_PACKAGE);
        // Add default settings rro package
        mApplicationConfigMap.put(AutoConfigConstants.SETTINGS_RRO_PACKAGE, SETTING_RRO_PACKAGE);
        // Add default open settings (full settings) command
        mApplicationConfigMap.put(AutoConfigConstants.OPEN_SETTINGS_COMMAND, OPEN_SETTINGS_COMMAND);
        // Add default open quick settings command
        mApplicationConfigMap.put(
                AutoConfigConstants.OPEN_QUICK_SETTINGS_COMMAND, OPEN_QUICK_SETTINGS_COMMAND);
        // Add default settings spli screen UI config
        mApplicationConfigMap.put(AutoConfigConstants.SPLIT_SCREEN_UI, "TRUE");
    }

    private void loadDefaultSettingOptions(Map<String, String[]> mSettingsOptionsMap) {
        mSettingsOptionsMap.put(
                AutoConfigConstants.DISPLAY_SETTINGS, new String[] {"Brightness level"});
        mSettingsOptionsMap.put(
                AutoConfigConstants.SOUND_SETTINGS, new String[] {"Media volume", "Alarm volume"});
        mSettingsOptionsMap.put(AutoConfigConstants.NETWORK_AND_INTERNET_SETTINGS, new String[] {});
        mSettingsOptionsMap.put(AutoConfigConstants.BLUETOOTH_SETTINGS, new String[] {});
        mSettingsOptionsMap.put(AutoConfigConstants.APPS_SETTINGS, new String[] {});
        mSettingsOptionsMap.put(
                AutoConfigConstants.DATE_AND_TIME_SETTINGS,
                new String[] {"Set time automatically", "Set time zone automatically"});
        mSettingsOptionsMap.put(
                AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS, new String[] {"Add a profile"});
        mSettingsOptionsMap.put(
                AutoConfigConstants.SYSTEM_SETTINGS, new String[] {"System update"});
        mSettingsOptionsMap.put(AutoConfigConstants.SECURITY_SETTINGS, new String[] {});
    }

    private void loadDefaultSettingPaths(Map<String, String[]> mSettingsPathMap) {
        mSettingsPathMap.put(AutoConfigConstants.DISPLAY_SETTINGS, new String[] {"Display"});
        mSettingsPathMap.put(AutoConfigConstants.SOUND_SETTINGS, new String[] {"Sound"});
        mSettingsPathMap.put(
                AutoConfigConstants.NETWORK_AND_INTERNET_SETTINGS,
                new String[] {"Network & internet"});
        mSettingsPathMap.put(AutoConfigConstants.BLUETOOTH_SETTINGS, new String[] {"Bluetooth"});
        mSettingsPathMap.put(AutoConfigConstants.APPS_SETTINGS, new String[] {"Apps"});
        mSettingsPathMap.put(
                AutoConfigConstants.DATE_AND_TIME_SETTINGS, new String[] {"System", "Date & time"});
        mSettingsPathMap.put(
                AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS, new String[] {"Profiles & accounts"});
        mSettingsPathMap.put(AutoConfigConstants.SYSTEM_SETTINGS, new String[] {"System"});
        mSettingsPathMap.put(AutoConfigConstants.SECURITY_SETTINGS, new String[] {"Security"});
    }

    private void loadDefaultSettingResourceConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        // Full Settings Config
        loadDefaultFullSettingsConfig(mSettingsConfigMap);

        // Quick Settings Config
        loadDefaultQuickSettingsConfig(mSettingsConfigMap);

        // Display Settings Config
        loadDefaultDisplaySettingsConfig(mSettingsConfigMap);

        // Sound Settings Config
        loadDefaultSoundSettingsConfig(mSettingsConfigMap);

        // Network And Internet Settings Config
        loadDefaultNetworkSettingsConfig(mSettingsConfigMap);

        // Bluetooth Settings Config
        loadDefaultBluetoothSettingsConfig(mSettingsConfigMap);

        // App and Notifications Settings Config
        loadDefaultAppAndNotificationsSettingsConfig(mSettingsConfigMap);

        // Date and Time Settings Config
        loadDefaultDateAndTimeSettingsConfig(mSettingsConfigMap);

        // System Settings Config
        loadDefaultSystemSettingsConfig(mSettingsConfigMap);

        // Account Settings Config
        loadDefaultAccountSettingsConfig(mSettingsConfigMap);

        // Security Settings Config
        loadDefaultSecuritySettingsConfig(mSettingsConfigMap);
    }

    private void loadDefaultFullSettingsConfig(Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration fullSettingsConfiguration = new AutoConfiguration();
        fullSettingsConfiguration.addResource(
                AutoConfigConstants.PAGE_TITLE,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "car_ui_toolbar_title",
                        SETTING_APP_PACKAGE));
        fullSettingsConfiguration.addResource(
                AutoConfigConstants.SEARCH,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Search"));
        fullSettingsConfiguration.addResource(
                AutoConfigConstants.SEARCH_BOX,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "car_ui_toolbar_search_bar",
                        SETTING_INTELLIGENCE_PACKAGE));
        fullSettingsConfiguration.addResource(
                AutoConfigConstants.SEARCH_RESULTS,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "recycler_view",
                        SETTING_INTELLIGENCE_PACKAGE));
        fullSettingsConfiguration.addResource(
                AutoConfigConstants.UP_BUTTON,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Scroll up"));
        fullSettingsConfiguration.addResource(
                AutoConfigConstants.DOWN_BUTTON,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Scroll down"));
        mSettingsConfigMap.put(AutoConfigConstants.FULL_SETTINGS, fullSettingsConfiguration);
    }

    private void loadDefaultQuickSettingsConfig(Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration quickSettingsConfiguration = new AutoConfiguration();
        quickSettingsConfiguration.addResource(
                AutoConfigConstants.OPEN_MORE_SETTINGS,
                new AutoConfigResource(
                        AutoConfigConstants.DESCRIPTION, "More", SETTING_APP_PACKAGE));
        quickSettingsConfiguration.addResource(
                AutoConfigConstants.NIGHT_MODE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Night mode"));
        mSettingsConfigMap.put(AutoConfigConstants.QUICK_SETTINGS, quickSettingsConfiguration);
    }

    private void loadDefaultDisplaySettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration displaySettingsConfiguration = new AutoConfiguration();
        displaySettingsConfiguration.addResource(
                AutoConfigConstants.BRIGHTNESS_LEVEL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "seekbar", SETTING_APP_PACKAGE));
        mSettingsConfigMap.put(AutoConfigConstants.DISPLAY_SETTINGS, displaySettingsConfiguration);
    }

    private void loadDefaultSoundSettingsConfig(Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration soundSettingsConfiguration = new AutoConfiguration();
        soundSettingsConfiguration.addResource(
                AutoConfigConstants.SAVE_BUTTON,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Save"));
        mSettingsConfigMap.put(AutoConfigConstants.SOUND_SETTINGS, soundSettingsConfiguration);
    }

    private void loadDefaultNetworkSettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration networkSettingsConfiguration = new AutoConfiguration();
        networkSettingsConfiguration.addResource(
                AutoConfigConstants.TOGGLE_WIFI,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Wi‑Fi toggle switch"));
        networkSettingsConfiguration.addResource(
                AutoConfigConstants.TOGGLE_HOTSPOT,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "car_ui_secondary_action_concrete",
                        SETTING_APP_PACKAGE));
        mSettingsConfigMap.put(
                AutoConfigConstants.NETWORK_AND_INTERNET_SETTINGS, networkSettingsConfiguration);
    }

    private void loadDefaultBluetoothSettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration bluetoothSettingsConfiguration = new AutoConfiguration();
        bluetoothSettingsConfiguration.addResource(
                AutoConfigConstants.TOGGLE_BLUETOOTH,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Bluetooth toggle switch"));
        mSettingsConfigMap.put(
                AutoConfigConstants.BLUETOOTH_SETTINGS, bluetoothSettingsConfiguration);
    }

    private void loadDefaultAppAndNotificationsSettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration appsAndNotificationsSettingsConfiguration = new AutoConfiguration();
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.PERMISSIONS_PAGE_TITLE,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "car_ui_toolbar_title",
                        PERMISSIONS_PACKAGE));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.VIEW_ALL,
                new AutoConfigResource(AutoConfigConstants.TEXT_CONTAINS, "View all"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.ENABLE_DISABLE_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "button1Text", SETTING_APP_PACKAGE));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.DISABLE_BUTTON_TEXT,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Disable"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.ENABLE_BUTTON_TEXT,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Enable"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.DISABLE_APP_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "DISABLE APP"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.FORCE_STOP_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Force stop"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.OK_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "OK"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.PERMISSIONS_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Permissions?"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.ALLOW_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Allow"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.DONT_ALLOW_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Don’t allow"));
        appsAndNotificationsSettingsConfiguration.addResource(
                AutoConfigConstants.DONT_ALLOW_ANYWAY_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Don’t allow anyway"));
        mSettingsConfigMap.put(
                AutoConfigConstants.APPS_SETTINGS, appsAndNotificationsSettingsConfiguration);
    }

    private void loadDefaultDateAndTimeSettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration dateAndTimeSettingsConfiguration = new AutoConfiguration();
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.SET_TIME_AUTOMATICALLY,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Set time automatically"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.SET_TIME_ZONE_AUTOMATICALLY,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Set time zone automatically"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.SET_DATE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Set date"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.SET_TIME,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Set time"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.SELECT_TIME_ZONE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Select time zone"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.USE_24_HOUR_FORMAT,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Use 24-hour format"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.OK_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "toolbar_menu_item_0",
                        SETTING_APP_PACKAGE));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.NUMBER_PICKER_WIDGET,
                new AutoConfigResource(AutoConfigConstants.CLASS, "android.widget.NumberPicker"));
        dateAndTimeSettingsConfiguration.addResource(
                AutoConfigConstants.EDIT_TEXT_WIDGET,
                new AutoConfigResource(AutoConfigConstants.CLASS, "android.widget.EditText"));
        mSettingsConfigMap.put(
                AutoConfigConstants.DATE_AND_TIME_SETTINGS, dateAndTimeSettingsConfiguration);
    }

    private void loadDefaultSystemSettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration systemSettingsConfiguration = new AutoConfiguration();
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.ABOUT_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "About"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.RESET_OPTIONS_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Reset options"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.LANGUAGES_AND_INPUT_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Languages & input"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.DEVICE_MODEL,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Model & hardware"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.ANDROID_VERSION,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Android version"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.ANDROID_SECURITY_PATCH_LEVEL,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Android security patch level"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.KERNEL_VERSION,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Kernel Version"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.BUILD_NUMBER,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Build number"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.RECYCLER_VIEW_WIDGET,
                new AutoConfigResource(
                        AutoConfigConstants.CLASS, "androidx.recyclerview.widget.RecyclerView"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.RESET_NETWORK,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Reset network"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.RESET_SETTINGS,
                new AutoConfigResource(AutoConfigConstants.TEXT, "RESET SETTINGS"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.RESET_APP_PREFERENCES,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Reset app preferences"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.RESET_APPS,
                new AutoConfigResource(AutoConfigConstants.TEXT, "RESET APPS"));
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.LANGUAGES_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Languages"));
        // Selected Language Spanish by default
        systemSettingsConfiguration.addResource(
                AutoConfigConstants.LANGUAGES_MENU_IN_SELECTED_LANGUAGE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Idiomas"));
        mSettingsConfigMap.put(AutoConfigConstants.SYSTEM_SETTINGS, systemSettingsConfiguration);
    }

    private void loadDefaultAccountSettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration accountSettingsConfiguration = new AutoConfiguration();
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.ADD_ACCOUNT,
                new AutoConfigResource(AutoConfigConstants.TEXT, "ADD ACCOUNT"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.ADD_GOOGLE_ACCOUNT,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Google"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.SIGN_IN_ON_CAR_SCREEN,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Sign in on car screen"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.GOOGLE_SIGN_IN_SCREEN,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Sign in to your Google Account"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.ENTER_EMAIL,
                new AutoConfigResource(AutoConfigConstants.CLASS, "android.widget.EditText"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.ENTER_PASSWORD,
                new AutoConfigResource(AutoConfigConstants.CLASS, "android.widget.EditText"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.NEXT_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Next"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.DONE_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Done"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.REMOVE_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Remove"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.REMOVE_ACCOUNT_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Remove Account"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.MANAGE_OTHER_PROFILES,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Manage other profiles"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.ADD_PROFILE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Add.*profile"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.OK, new AutoConfigResource(AutoConfigConstants.TEXT, "OK"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.DELETE_SELF,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Delete this profile"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.DELETE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Delete"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.MAKE_ADMIN,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Make Admin"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.MAKE_ADMIN_CONFIRM,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Yes, make admin"));
        accountSettingsConfiguration.addResource(
                AutoConfigConstants.TIME_PATTERN,
                new AutoConfigResource(
                        AutoConfigConstants.TEXT, "(1[012]|[1-9]):[0-5][0-9](\\s)?.*"));
        mSettingsConfigMap.put(
                AutoConfigConstants.PROFILE_ACCOUNT_SETTINGS, accountSettingsConfiguration);
    }

    private void loadDefaultSecuritySettingsConfig(
            Map<String, AutoConfiguration> mSettingsConfigMap) {
        AutoConfiguration securitySettingsConfiguration = new AutoConfiguration();
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.TITLE,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "car_ui_toolbar_title",
                        SETTING_APP_PACKAGE));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.CHOOSE_LOCK_TYPE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Choose a lock type"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.PROFILE_LOCK,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Profile lock"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.LOCK_TYPE_PASSWORD,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Password"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.LOCK_TYPE_PIN,
                new AutoConfigResource(AutoConfigConstants.TEXT, "PIN"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.LOCK_TYPE_NONE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "None"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.CONTINUE_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Continue"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.CONFIRM_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Confirm"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.ENTER_PASSWORD,
                new AutoConfigResource(AutoConfigConstants.CLASS, "android.widget.EditText"));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.PIN_PAD,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "pin_pad", SETTING_APP_PACKAGE));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.ENTER_PIN_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "key_enter", SETTING_APP_PACKAGE));
        securitySettingsConfiguration.addResource(
                AutoConfigConstants.REMOVE_BUTTON,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Remove"));
        mSettingsConfigMap.put(
                AutoConfigConstants.SECURITY_SETTINGS, securitySettingsConfiguration);
    }
}
