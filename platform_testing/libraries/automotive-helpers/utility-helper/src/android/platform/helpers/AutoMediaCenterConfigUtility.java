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

/** Configuration for Media Center Application */
public class AutoMediaCenterConfigUtility implements IAutoConfigUtility {
    private static final String LOG_TAG = AutoMediaCenterConfigUtility.class.getSimpleName();

    // Media Center Config
    private static final String MEDIA_CENTER_PACKAGE = "com.android.car.media";
    private static final String MEDIA_ACTIVITY =
            "com.android.bluetooth/"
                    + "com.android.bluetooth.avrcpcontroller.BluetoothMediaBrowserService";

    // Car Launcher For Reference Device
    private static final String CAR_LAUNCHER_PACKAGE = "com.android.car.carlauncher";

    // Config Map
    private Map<String, AutoConfiguration> mMediaCenterConfigMap;

    private static AutoMediaCenterConfigUtility sAutoMediaCenterConfigUtility = null;

    private AutoMediaCenterConfigUtility() {
        // Initialize Media Center Config Map
        mMediaCenterConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get Auto Media Center Config Utility Instance */
    public static IAutoConfigUtility getInstance() {
        if (sAutoMediaCenterConfigUtility == null) {
            sAutoMediaCenterConfigUtility = new AutoMediaCenterConfigUtility();
        }
        return sAutoMediaCenterConfigUtility;
    }

    /**
     * Get Path for given menu
     *
     * @param menu Menu Name
     */
    public String[] getPath(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Media Center Application");
    }

    /**
     * Add path for given Menu
     *
     * @param menu Menu Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        throw new UnsupportedOperationException(
                "Operation not supported for Media Center Application");
    }

    /**
     * Get available menu option
     *
     * @param menu Menu Name
     */
    public String[] getAvailableOptions(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Media Center Application");
    }

    /**
     * Add available menu options
     *
     * @param menu Menu Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        throw new UnsupportedOperationException(
                "Operation not supported for Media Center Application");
    }

    /**
     * Get Media Center Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mMediaCenterConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mMediaCenterConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate Media Center Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mMediaCenterConfigMap.get(configName) != null);
    }

    /**
     * Validate Media Center Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mMediaCenterConfigMap.get(configName) == null) {
            return false;
        }
        return (mMediaCenterConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Menu Options
     *
     * @param menu Menu Name
     */
    public boolean isValidOption(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Media Center Application");
    }

    /**
     * Validate Menu Path
     *
     * @param menu Menu Name
     */
    public boolean isValidPath(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Media Center Application");
    }

    /** Load default config for Media Center Application ( Based on Reference Device ) */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default Media Center Application Config
        loadDefaultMediaCenterAppConfig(mApplicationConfigMap);

        // Default Media Center Screen Config
        loadDefaultMediaCenterScreenConfig(mMediaCenterConfigMap);

        // Default Media Center on Home Screen Config
        loadDefaultMediaCenterOnHomeScreenConfig(mMediaCenterConfigMap);
    }

    private void loadDefaultMediaCenterAppConfig(Map<String, String> mApplicationConfigMap) {
        // Add default Media Center package
        mApplicationConfigMap.put(AutoConfigConstants.MEDIA_CENTER_PACKAGE, MEDIA_CENTER_PACKAGE);
        // Add default Media Activity ( Default is for Bluetooth Audio )
        mApplicationConfigMap.put(AutoConfigConstants.MEDIA_ACTIVITY, MEDIA_ACTIVITY);
    }

    private void loadDefaultMediaCenterScreenConfig(
            Map<String, AutoConfiguration> mMediaCenterConfigMap) {
        AutoConfiguration mediaCenterScreenConfiguration = new AutoConfiguration();
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.PLAY_PAUSE_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "play_pause_stop", MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.NEXT_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "skip_next", MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.PREVIOUS_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "skip_prev", MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.SHUFFLE_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "overflow_on", MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.PLAY_QUEUE_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "play_queue", MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.MINIMIZED_MEDIA_CONTROLS,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "minimized_playback_controls",
                        MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.TRACK_NAME,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "title", MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.TRACK_NAME_MINIMIZED_CONTROL,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID,
                        "minimized_control_bar_title",
                        MEDIA_CENTER_PACKAGE));
        mediaCenterScreenConfiguration.addResource(
                AutoConfigConstants.BACK_BUTTON,
                new AutoConfigResource(AutoConfigConstants.DESCRIPTION, "Back"));
        mMediaCenterConfigMap.put(
                AutoConfigConstants.MEDIA_CENTER_SCREEN, mediaCenterScreenConfiguration);
    }

    private void loadDefaultMediaCenterOnHomeScreenConfig(
            Map<String, AutoConfiguration> mMediaCenterConfigMap) {
        AutoConfiguration mediaCenterOnHomeScreenConfiguration = new AutoConfiguration();
        mediaCenterOnHomeScreenConfiguration.addResource(
                AutoConfigConstants.PLAY_PAUSE_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "play_pause_stop", CAR_LAUNCHER_PACKAGE));
        mediaCenterOnHomeScreenConfiguration.addResource(
                AutoConfigConstants.PREVIOUS_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "skip_prev", CAR_LAUNCHER_PACKAGE));
        mediaCenterOnHomeScreenConfiguration.addResource(
                AutoConfigConstants.NEXT_BUTTON,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "skip_next", CAR_LAUNCHER_PACKAGE));
        mediaCenterOnHomeScreenConfiguration.addResource(
                AutoConfigConstants.TRACK_NAME,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "title", CAR_LAUNCHER_PACKAGE));
        mMediaCenterConfigMap.put(
                AutoConfigConstants.MEDIA_CENTER_ON_HOME_SCREEN,
                mediaCenterOnHomeScreenConfiguration);
    }
}
