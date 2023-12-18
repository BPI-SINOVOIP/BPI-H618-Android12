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

/** Configuration for Notifications Application */
public class AutoNotificationsConfigUtility implements IAutoConfigUtility {
    private static final String LOG_TAG = AutoNotificationsConfigUtility.class.getSimpleName();

    // Notifications Config
    private static final String SYSTEM_UI_PACKAGE = "com.android.systemui";
    private static final String RECYCLER_VIEW_CLASS = "androidx.recyclerview.widget.RecyclerView";
    private static final String OPEN_NOTIFICATIONS_COMMAND = "service call statusbar 1";

    // Config Map
    private Map<String, AutoConfiguration> mNotificationConfigMap;

    private static AutoNotificationsConfigUtility sAutoNotificationsConfigInstance = null;

    private AutoNotificationsConfigUtility() {
        // Initialize Notifications Config Map
        mNotificationConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get instance of Auto Notifications Utility */
    public static IAutoConfigUtility getInstance() {
        if (sAutoNotificationsConfigInstance == null) {
            sAutoNotificationsConfigInstance = new AutoNotificationsConfigUtility();
        }
        return sAutoNotificationsConfigInstance;
    }

    /**
     * Get Path for given menu
     *
     * @param menu Menu Name
     */
    public String[] getPath(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Notifications Application");
    }

    /**
     * Add path for given Menu
     *
     * @param menu Menu Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        throw new UnsupportedOperationException(
                "Operation not supported for Notifications Application");
    }

    /**
     * Get available menu option
     *
     * @param menu Menu Name
     */
    public String[] getAvailableOptions(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Notifications Application");
    }

    /**
     * Add available menu options
     *
     * @param menu Menu Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        throw new UnsupportedOperationException(
                "Operation not supported for Notifications Application");
    }

    /**
     * Get Notifications Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mNotificationConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mNotificationConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate Notifications Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mNotificationConfigMap.get(configName) != null);
    }

    /**
     * Validate Notifications Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mNotificationConfigMap.get(configName) == null) {
            return false;
        }
        return (mNotificationConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Menu Options
     *
     * @param menu Menu Name
     */
    public boolean isValidOption(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Notifications Application");
    }

    /**
     * Validate Menu Path
     *
     * @param menu Menu Name
     */
    public boolean isValidPath(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Notifications Application");
    }

    /** Load default configuration for Notification application */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default Notification Application Config
        loadDefaultNotificationAppConfig(mApplicationConfigMap);

        // Default Expanded Notifications Screen Config
        loadDefaultExpandedNotificationsConfig(mNotificationConfigMap);
    }

    private void loadDefaultNotificationAppConfig(Map<String, String> mApplicationConfigMap) {
        // Add default command to open Notifications
        mApplicationConfigMap.put(
                AutoConfigConstants.OPEN_NOTIFICATIONS_COMMAND, OPEN_NOTIFICATIONS_COMMAND);
        // Add default recyler view class
        mApplicationConfigMap.put(AutoConfigConstants.RECYCLER_VIEW_CLASS, RECYCLER_VIEW_CLASS);
    }

    private void loadDefaultExpandedNotificationsConfig(
            Map<String, AutoConfiguration> mNotificationConfigMap) {
        AutoConfiguration expandedNotificationsConfig = new AutoConfiguration();
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.NOTIFICATION_LIST,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "notifications", SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.NOTIFICATION_VIEW,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "notification_view", SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.CLEAR_ALL_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "clear_all_button", SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.STATUS_BAR,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "car_top_bar", SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.APP_ICON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "app_icon", SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.APP_NAME,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "header_text", SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.NOTIFICATION_TITLE,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "notification_body_title",
                        SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.NOTIFICATION_BODY,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "notification_body_content",
                        SYSTEM_UI_PACKAGE));
        expandedNotificationsConfig.addResource(
                AutoConfigConstants.CARD_VIEW,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "card_view", SYSTEM_UI_PACKAGE));
        mNotificationConfigMap.put(
                AutoConfigConstants.EXPANDED_NOTIFICATIONS_SCREEN, expandedNotificationsConfig);
    }
}
