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

/** Configuration for Dialer/Phone Application */
public class AutoDialConfigUtility implements IAutoConfigUtility {
    private static final String LOG_TAG = AutoDialConfigUtility.class.getSimpleName();

    // Dialer/Phone Config
    private static final String DIAL_APP_PACKAGE = "com.android.car.dialer";
    private static final String PHONE_ACTIVITY = "com.android.car.dialer/.ui.TelecomActivity";
    private static final String OPEN_DIAL_PAD_COMMAND = "am start -a android.intent.action.DIAL";

    // Config Map
    private Map<String, AutoConfiguration> mDialConfigMap;

    private static AutoDialConfigUtility sAutoDialConfigInstance = null;

    private AutoDialConfigUtility() {
        // Initialize Dial Config Map
        mDialConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get instance of Auto Dial Utility */
    public static IAutoConfigUtility getInstance() {
        if (sAutoDialConfigInstance == null) {
            sAutoDialConfigInstance = new AutoDialConfigUtility();
        }
        return sAutoDialConfigInstance;
    }

    /**
     * Get Path for given menu
     *
     * @param menu Menu Name
     */
    public String[] getPath(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Phone/Dialer Application");
    }

    /**
     * Add path for given Menu
     *
     * @param menu Menu Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        throw new UnsupportedOperationException(
                "Operation not supported for Phone/Dialer Application");
    }

    /**
     * Get available menu option
     *
     * @param menu Menu Name
     */
    public String[] getAvailableOptions(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Phone/Dialer Application");
    }

    /**
     * Add available menu options
     *
     * @param menu Menu Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        throw new UnsupportedOperationException(
                "Operation not supported for Phone/Dialer Application");
    }

    /**
     * Get Phone/Dialer Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mDialConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mDialConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate Phone/Dialer Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mDialConfigMap.get(configName) != null);
    }

    /**
     * Validate Phone/Dialer Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mDialConfigMap.get(configName) == null) {
            return false;
        }
        return (mDialConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Menu Options
     *
     * @param menu Menu Name
     */
    public boolean isValidOption(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Phone/Dialer Application");
    }

    /**
     * Validate Menu Path
     *
     * @param menu Menu Name
     */
    public boolean isValidPath(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Phone/Dialer Application");
    }

    /** Load default configuration for Dialer/Phone application */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default Dial Application Config
        loadDefaultDialAppConfig(mApplicationConfigMap);

        // Default In Call Screen Config
        loadDefaultInCallConfig(mDialConfigMap);

        // Default Dial Pad Screen Config
        loadDefaultDialPadConfig(mDialConfigMap);

        // Default Contacts Screen Config
        loadDefaultContactsConfig(mDialConfigMap);

        // Default Call History Screen Config
        loadDefaultCallHistoryConfig(mDialConfigMap);

        // Default Favorites Screen Config
        loadDefaultFavoritesConfig(mDialConfigMap);
    }

    private void loadDefaultDialAppConfig(Map<String, String> mApplicationConfigMap) {
        // Add default Dial package
        mApplicationConfigMap.put(AutoConfigConstants.DIAL_PACKAGE, DIAL_APP_PACKAGE);
        // Add default Phone package
        mApplicationConfigMap.put(AutoConfigConstants.PHONE_ACTIVITY, PHONE_ACTIVITY);
        // Add default command to open Dial Pad
        mApplicationConfigMap.put(AutoConfigConstants.OPEN_DIAL_PAD_COMMAND, OPEN_DIAL_PAD_COMMAND);
    }

    private void loadDefaultInCallConfig(Map<String, AutoConfiguration> mDialConfigMap) {
        AutoConfiguration inCallScreenConfiguration = new AutoConfiguration();
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.DIALED_CONTACT_TITLE,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "user_profile_title", DIAL_APP_PACKAGE));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.DIALED_CONTACT_NUMBER,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "user_profile_phone_number",
                        DIAL_APP_PACKAGE));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.END_CALL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "end_call_button", DIAL_APP_PACKAGE));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.MUTE_CALL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "mute_button", DIAL_APP_PACKAGE));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.SWITCH_TO_DIAL_PAD,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "toggle_dialpad_button",
                        DIAL_APP_PACKAGE));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.CHANGE_VOICE_CHANNEL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "voice_channel_view", DIAL_APP_PACKAGE));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.VOICE_CHANNEL_CAR,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Car speakers"));
        inCallScreenConfiguration.addResource(
                AutoConfigConstants.VOICE_CHANNEL_PHONE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Phone"));
        mDialConfigMap.put(AutoConfigConstants.IN_CALL_VIEW, inCallScreenConfiguration);
    }

    private void loadDefaultDialPadConfig(Map<String, AutoConfiguration> mDialConfigMap) {
        AutoConfiguration dialPadScreenConfiguration = new AutoConfiguration();
        dialPadScreenConfiguration.addResource(
                AutoConfigConstants.DIAL_PAD_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Dial.?pad"));
        dialPadScreenConfiguration.addResource(
                AutoConfigConstants.DIAL_PAD_FRAGMENT,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "dialpad_fragment", DIAL_APP_PACKAGE));
        dialPadScreenConfiguration.addResource(
                AutoConfigConstants.DIALED_NUMBER,
                new AutoConfigResource(AutoConfigConstants.RESOURCE_ID, "title", DIAL_APP_PACKAGE));
        dialPadScreenConfiguration.addResource(
                AutoConfigConstants.MAKE_CALL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "call_button", DIAL_APP_PACKAGE));
        dialPadScreenConfiguration.addResource(
                AutoConfigConstants.DELETE_NUMBER,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "delete_button", DIAL_APP_PACKAGE));
        mDialConfigMap.put(AutoConfigConstants.DIAL_PAD_VIEW, dialPadScreenConfiguration);
    }

    private void loadDefaultContactsConfig(Map<String, AutoConfiguration> mDialConfigMap) {
        AutoConfiguration contactsScreenConfig = new AutoConfiguration();
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACTS_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Contacts"));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_INFO,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "call_action_id", DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_NAME,
                new AutoConfigResource(AutoConfigConstants.RESOURCE_ID, "title", DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_DETAIL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "show_contact_detail_id",
                        DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.ADD_CONTACT_TO_FAVORITE,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "contact_details_favorite_button",
                        DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.SEARCH_CONTACT,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "menu_item_search", DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_SEARCH_BAR,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "car_ui_toolbar_search_bar",
                        DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.SEARCH_RESULT,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "contact_name", DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_SETTINGS,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "menu_item_setting", DIAL_APP_PACKAGE));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_ORDER,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Contact order"));
        contactsScreenConfig.addResource(
                AutoConfigConstants.SORT_BY_FIRST_NAME,
                new AutoConfigResource(AutoConfigConstants.TEXT, "First name"));
        contactsScreenConfig.addResource(
                AutoConfigConstants.SORT_BY_LAST_NAME,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Last name"));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_TYPE_WORK,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Work"));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_TYPE_MOBILE,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Mobile"));
        contactsScreenConfig.addResource(
                AutoConfigConstants.CONTACT_TYPE_HOME,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Home"));
        mDialConfigMap.put(AutoConfigConstants.CONTACTS_VIEW, contactsScreenConfig);
    }

    private void loadDefaultCallHistoryConfig(Map<String, AutoConfiguration> mDialConfigMap) {
        AutoConfiguration callHistoryScreenConfiguration = new AutoConfiguration();
        callHistoryScreenConfiguration.addResource(
                AutoConfigConstants.CALL_HISTORY_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Recents"));
        callHistoryScreenConfiguration.addResource(
                AutoConfigConstants.CALL_HISTORY_INFO,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "call_action_id", DIAL_APP_PACKAGE));
        mDialConfigMap.put(AutoConfigConstants.CALL_HISTORY_VIEW, callHistoryScreenConfiguration);
    }

    private void loadDefaultFavoritesConfig(Map<String, AutoConfiguration> mDialConfigMap) {
        AutoConfiguration favoritesScreenConfiguration = new AutoConfiguration();
        favoritesScreenConfiguration.addResource(
                AutoConfigConstants.FAVORITES_MENU,
                new AutoConfigResource(AutoConfigConstants.TEXT, "Favo.?rite.?"));
        mDialConfigMap.put(AutoConfigConstants.FAVORITES_VIEW, favoritesScreenConfiguration);
    }
}
