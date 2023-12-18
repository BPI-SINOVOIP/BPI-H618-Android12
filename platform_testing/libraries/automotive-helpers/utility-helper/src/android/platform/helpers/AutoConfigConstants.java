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

// Automotive Configuration Constants
public class AutoConfigConstants {
    // Resource Types
    public static final String RESOURCE_ID = "RESOURCE_ID";
    public static final String TEXT = "TEXT";
    public static final String TEXT_CONTAINS = "TEXT_CONTAINS";
    public static final String DESCRIPTION = "DESCRIPTION";
    public static final String CLASS = "CLASS";

    // App Grid
    public static final String APP_GRID = "APP_GRID";
    public static final String APP_GRID_ACTIVITY = "APP_GRID_ACTIVITY";
    public static final String APP_GRID_PACKAGE = "APP_GRID_PACKAGE";
    public static final String APP_GRID_VIEW = "APP_GRID_VIEW";
    public static final String APP_GRID_VIEW_ID = "APP_GRID_VIEW_ID";
    public static final String APPLICATION_NAME = "APPLICATION_NAME";
    public static final String OPEN_APP_GRID_COMMAND = "OPEN_APP_GRID_COMMAND";

    // Home
    public static final String BOTTOM_CARD = "BOTTOM_CARD";
    public static final String HOME = "HOME";
    public static final String HOME_PACKAGE = "HOME_PACKAGE";
    public static final String HOME_VIEW = "HOME_VIEW";
    public static final String MAP_CARD = "MAP_CARD";
    public static final String TOP_CARD = "TOP_CARD";

    // Lock Screen
    public static final String LOCK_SCREEN = "LOCK_SCREEN";
    public static final String LOCK_SCREEN_PACKAGE = "LOCK_SCREEN_PACKAGE";
    public static final String LOCK_SCREEN_VIEW = "LOCK_SCREEN_VIEW";
    public static final String ENTER_KEY = "ENTER_KEY";

    // SETTINGS
    public static final String SETTINGS = "SETTINGS";
    public static final String SETTINGS_TITLE_TEXT = "SETTINGS_TITLE_TEXT";
    public static final String SETTINGS_PACKAGE = "SETTINGS_PACKAGE";
    public static final String SETTINGS_RRO_PACKAGE = "SETTINGS_RRO_PACKAGE";
    public static final String OPEN_SETTINGS_COMMAND = "OPEN_SETTINGS_COMMAND";
    public static final String OPEN_QUICK_SETTINGS_COMMAND = "OPEN_QUICK_SETTINGS_COMMAND";
    public static final String SPLIT_SCREEN_UI = "SPLIT_SCREEN_UI";
    // Full Settings
    public static final String FULL_SETTINGS = "FULL_SETTINGS";
    public static final String PAGE_TITLE = "PAGE_TITLE";
    public static final String SEARCH = "SEARCH";
    public static final String SEARCH_BOX = "SEARCH_BOX";
    public static final String SEARCH_RESULTS = "SEARCH_RESULTS";
    public static final String UP_BUTTON = "UP_BUTTON";
    public static final String DOWN_BUTTON = "DOWN_BUTTON";
    // Quick Settings
    public static final String QUICK_SETTINGS = "QUICK_SETTINGS";
    public static final String OPEN_MORE_SETTINGS = "OPEN_MORE_SETTINGS";
    public static final String NIGHT_MODE = "NIGHT_MODE";
    // Display
    public static final String DISPLAY_SETTINGS = "DISPLAY";
    public static final String BRIGHTNESS_LEVEL = "BRIGHTNESS_LEVEL";
    // Sound
    public static final String SOUND_SETTINGS = "SOUND";
    public static final String SAVE_BUTTON = "SAVE";
    // Network and Internet
    public static final String NETWORK_AND_INTERNET_SETTINGS = "NETWORK_AND_INTERNET";
    public static final String TOGGLE_WIFI = "TOGGLE_WIFI";
    public static final String TOGGLE_HOTSPOT = "TOGGLE_HOTSPOT";
    // Bluetooth
    public static final String BLUETOOTH_SETTINGS = "BLUETOOTH";
    public static final String TOGGLE_BLUETOOTH = "TOGGLE_BLUETOOTH";
    // Apps
    public static final String APPS_SETTINGS = "APPS";
    public static final String PERMISSIONS_PAGE_TITLE = "PERMISSIONS_PAGE_TITLE";
    public static final String VIEW_ALL = "VIEW_ALL";
    public static final String ENABLE_DISABLE_BUTTON = "ENABLE_DISABLE_BUTTON";
    public static final String DISABLE_BUTTON_TEXT = "DISABLE_BUTTON_TEXT";
    public static final String ENABLE_BUTTON_TEXT = "ENABLE_BUTTON_TEXT";
    public static final String DISABLE_APP_BUTTON = "DISABLE_APP_BUTTON";
    public static final String FORCE_STOP_BUTTON = "FORCE_STOP_BUTTON";
    public static final String OK_BUTTON = "OK_BUTTON";
    public static final String PERMISSIONS_MENU = "PERMISSIONS_MENU";
    public static final String ALLOW_BUTTON = "ALLOW_BUTTON";
    public static final String DONT_ALLOW_BUTTON = "DONT_ALLOW_BUTTON";
    public static final String DONT_ALLOW_ANYWAY_BUTTON = "DONT_ALLOW_ANYWAY_BUTTON";
    // Date and Time
    public static final String DATE_AND_TIME_SETTINGS = "DATE_AND_TIME";
    public static final String SET_TIME_AUTOMATICALLY = "SET_TIME_AUTOMATICALLY";
    public static final String SET_TIME_ZONE_AUTOMATICALLY = "SET_TIME_ZONE_AUTOMATICALLY";
    public static final String SET_DATE = "SET_DATE";
    public static final String SET_TIME = "SET_TIME";
    public static final String SELECT_TIME_ZONE = "SELECT_TIME_ZONE";
    public static final String USE_24_HOUR_FORMAT = "USE_24_HOUR_FORMAT";
    // OK_BUTTON from Apps and Info Settings
    public static final String NUMBER_PICKER_WIDGET = "NUMBER_PICKER_WIDGET";
    public static final String EDIT_TEXT_WIDGET = "EDIT_TEXT_WIDGET";
    // System
    public static final String SYSTEM_SETTINGS = "SYSTEM";
    public static final String ABOUT_MENU = "ABOUT_MENU";
    public static final String RESET_OPTIONS_MENU = "RESET_OPTIONS_MENU";
    public static final String LANGUAGES_AND_INPUT_MENU = "LANGUAGES_AND_INPUT_MENU";
    public static final String DEVICE_MODEL = "DEVICE_MODEL";
    public static final String ANDROID_VERSION = "ANDROID_VERSION";
    public static final String ANDROID_SECURITY_PATCH_LEVEL = "ANDROID_SECURITY_PATCH_LEVEL";
    public static final String KERNEL_VERSION = "KERNEL_VERSION";
    public static final String BUILD_NUMBER = "BUILD_NUMBER";
    public static final String RECYCLER_VIEW_WIDGET = "RECYCLER_VIEW_WIDGET";
    public static final String RESET_NETWORK = "RESET_NETWORK";
    public static final String RESET_SETTINGS = "RESET_SETTINGS";
    public static final String RESET_APP_PREFERENCES = "RESET_APP_PREFERENCES";
    public static final String RESET_APPS = "RESET_APPS";
    public static final String LANGUAGES_MENU = "LANGUAGES_MENU";
    public static final String LANGUAGES_MENU_IN_SELECTED_LANGUAGE =
            "LANGUAGES_MENU_IN_SELECTED_LANGUAGE";

    // MU profiles
    public static final String MANAGE_OTHER_PROFILES = "MANAGE_OTHER_PROFILES";
    public static final String ADD_PROFILE = "ADD_PROFILE";
    public static final String OK = "OK";
    public static final String DELETE_SELF = "DELETE_SELF";
    public static final String DELETE = "DELETE";
    public static final String MAKE_ADMIN = "MAKE_ADMIN";
    public static final String MAKE_ADMIN_CONFIRM = "MAKE_ADMIN_CONFIRM";
    public static final String TIME_PATTERN = "TIME_PATTERN";

    // Accounts
    public static final String PROFILE_ACCOUNT_SETTINGS = "PROFILES_ACCOUNTS";
    public static final String ADD_ACCOUNT = "ADD_ACCOUNT";
    public static final String ADD_GOOGLE_ACCOUNT = "ADD_GOOGLE_ACCOUNT";
    public static final String SIGN_IN_ON_CAR_SCREEN = "SIGN_IN_ON_CAR_SCREEN";
    public static final String GOOGLE_SIGN_IN_SCREEN = "GOOGLE_SIGN_IN_SCREEN";
    public static final String ENTER_EMAIL = "ENTER_EMAIL";
    // Same ENTER_PASSWORD is applicable for Security Settings
    public static final String ENTER_PASSWORD = "ENTER_PASSWORD";
    // Same NEXT_BUTTON is applicable for Media Center
    public static final String NEXT_BUTTON = "NEXT_BUTTON";
    public static final String DONE_BUTTON = "DONE_BUTTON";
    // Same REMOVE_BUTTON is applicable for Security Settings
    public static final String REMOVE_BUTTON = "REMOVE_BUTTON";
    public static final String REMOVE_ACCOUNT_BUTTON = "REMOVE_ACCOUNT_BUTTON";
    // Security
    public static final String SECURITY_SETTINGS = "SECURITY";
    public static final String TITLE = "TITLE";
    public static final String CHOOSE_LOCK_TYPE = "CHOOSE_LOCK_TYPE";
    public static final String PROFILE_LOCK = "PROFILE_LOCK";
    public static final String LOCK_TYPE_PASSWORD = "LOCK_TYPE_PASSWORD";
    public static final String LOCK_TYPE_PIN = "LOCK_TYPE_PIN";
    public static final String LOCK_TYPE_NONE = "LOCK_TYPE_NONE";
    public static final String CONTINUE_BUTTON = "CONTINUE_BUTTON";
    public static final String CONFIRM_BUTTON = "CONFIRM_BUTTON";
    // ENTER_PASSWORD from Account Settings
    public static final String PIN_PAD = "PIN_PAD";
    public static final String ENTER_PIN_BUTTON = "ENTER_PIN_BUTTON";
    // REMOVE_BUTTON from Account Settings

    // PHONE
    public static final String PHONE = "PHONE";
    public static final String DIAL_PACKAGE = "DIAL_PACKAGE";
    public static final String OPEN_DIAL_PAD_COMMAND = "OPEN_DIAL_PAD_COMMAND";
    public static final String PHONE_ACTIVITY = "PHONE_ACTIVITY";
    // In Call Screen
    public static final String IN_CALL_VIEW = "IN_CALL_VIEW";
    public static final String DIALED_CONTACT_TITLE = "DIALED_CONTACT_TITLE";
    public static final String DIALED_CONTACT_NUMBER = "DIALED_CONTACT_NUMBER";
    public static final String END_CALL = "END_CALL";
    public static final String MUTE_CALL = "MUTE_CALL";
    public static final String SWITCH_TO_DIAL_PAD = "SWITCH_TO_DIAL_PAD";
    public static final String CHANGE_VOICE_CHANNEL = "CHANGE_VOICE_CHANNEL";
    public static final String VOICE_CHANNEL_CAR = "VOICE_CHANNEL_CAR";
    public static final String VOICE_CHANNEL_PHONE = "VOICE_CHANNEL_PHONE";
    // Dial Pad Screen
    public static final String DIAL_PAD_VIEW = "DIAL_PAD_VIEW";
    public static final String DIAL_PAD_MENU = "DIAL_PAD_MENU";
    public static final String DIAL_PAD_FRAGMENT = "DIAL_PAD_FRAGMENT";
    public static final String DIALED_NUMBER = "DIALED_NUMBER";
    public static final String MAKE_CALL = "MAKE_CALL";
    public static final String DELETE_NUMBER = "DELETE_NUMBER";
    // Contacts Screen
    public static final String CONTACTS_VIEW = "CONTACTS_VIEW";
    public static final String CONTACTS_MENU = "CONTACTS_MENU";
    public static final String CONTACT_INFO = "CONTACT_INFO";
    public static final String CONTACT_NAME = "CONTACT_NAME";
    public static final String CONTACT_DETAIL = "CONTACT_DETAIL";
    public static final String ADD_CONTACT_TO_FAVORITE = "ADD_CONTACT_TO_FAVORITE";
    public static final String SEARCH_CONTACT = "SEARCH_CONTACT";
    public static final String CONTACT_SEARCH_BAR = "CONTACT_SEARCH_BAR";
    public static final String SEARCH_RESULT = "SEARCH_RESULT";
    public static final String CONTACT_SETTINGS = "CONTACT_SETTINGS";
    public static final String CONTACT_ORDER = "CONTACT_ORDER";
    public static final String SORT_BY_FIRST_NAME = "SORT_BY_FIRST_NAME";
    public static final String SORT_BY_LAST_NAME = "SORT_BY_LAST_NAME";
    public static final String CONTACT_TYPE_WORK = "CONTACT_TYPE_WORK";
    public static final String CONTACT_TYPE_MOBILE = "CONTACT_TYPE_MOBILE";
    public static final String CONTACT_TYPE_HOME = "CONTACT_TYPE_HOME";
    // Call History Screen
    public static final String CALL_HISTORY_VIEW = "CALL_HISTORY_VIEW";
    public static final String CALL_HISTORY_MENU = "CALL_HISTORY_MENU";
    public static final String CALL_HISTORY_INFO = "CALL_HISTORY_INFO";
    // Favorites Screen
    public static final String FAVORITES_VIEW = "FAVORITES_VIEW";
    public static final String FAVORITES_MENU = "FAVORITES_MENU";

    // Notifications
    public static final String NOTIFICATIONS = "NOTIFICATIONS";
    public static final String OPEN_NOTIFICATIONS_COMMAND = "OPEN_NOTIFICATIONS_COMMAND";
    public static final String RECYCLER_VIEW_CLASS = "RECYCLER_VIEW_CLASS";
    // Expanded Notifications Screen
    public static final String EXPANDED_NOTIFICATIONS_SCREEN = "EXPANDED_NOTIFICATIONS_SCREEN";
    public static final String NOTIFICATION_LIST = "NOTIFICATION_LIST";
    public static final String NOTIFICATION_VIEW = "NOTIFICATION_VIEW";
    public static final String CLEAR_ALL_BUTTON = "CLEAR_ALL_BUTTON";
    public static final String STATUS_BAR = "STATUS_BAR";
    public static final String APP_ICON = "APP_ICON";
    public static final String APP_NAME = "APP_NAME";
    public static final String NOTIFICATION_TITLE = "NOTIFICATION_TITLE";
    public static final String NOTIFICATION_BODY = "NOTIFICATION_BODY";
    public static final String CARD_VIEW = "CARD_VIEW";

    // Media Center
    public static final String MEDIA_CENTER = "MEDIA_CENTER";
    public static final String MEDIA_CENTER_PACKAGE = "MEDIA_CENTER_PACKAGE";
    public static final String MEDIA_ACTIVITY = "MEDIA_ACTIVITY";
    // Media Center Screen
    public static final String MEDIA_CENTER_SCREEN = "MEDIA_CENTER_SCREEN";
    public static final String PLAY_PAUSE_BUTTON = "PLAY_PAUSE_BUTTON";
    // NEXT_BUTTON from Account Settings
    public static final String PREVIOUS_BUTTON = "PREVIOUS_BUTTON";
    public static final String SHUFFLE_BUTTON = "SHUFFLE_BUTTON";
    public static final String PLAY_QUEUE_BUTTON = "PLAY_QUEUE_BUTTON";
    public static final String MINIMIZED_MEDIA_CONTROLS = "MINIMIZED_MEDIA_CONTROLS";
    public static final String TRACK_NAME = "TRACK_NAME";
    public static final String TRACK_NAME_MINIMIZED_CONTROL = "TRACK_NAME_MINIMIZED_CONTROL";
    public static final String BACK_BUTTON = "BACK_BUTTON";
    // Media Center On Home Screen
    public static final String MEDIA_CENTER_ON_HOME_SCREEN = "MEDIA_CENTER_ON_HOME_SCREEN";
}
