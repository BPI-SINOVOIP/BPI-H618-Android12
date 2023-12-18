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

/** Configuration for Home app */
public class AutoHomeConfigUtility implements IAutoConfigUtility {

    private static final String LOG_TAG = AutoHomeConfigUtility.class.getSimpleName();

    // HOME Config
    private static final String HOME_PACKAGE = "com.android.car.carlauncher";

    // HOME Map
    private Map<String, AutoConfiguration> mHomeConfigMap;

    private static AutoHomeConfigUtility sAutoHomeConfigInstance = null;

    private AutoHomeConfigUtility() {
        // Initialize Home Config Map
        mHomeConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get instance of Auto Home Utility */
    public static IAutoConfigUtility getInstance() {
        if (sAutoHomeConfigInstance == null) {
            sAutoHomeConfigInstance = new AutoHomeConfigUtility();
        }
        return sAutoHomeConfigInstance;
    }

    /**
     * Get Path for given menu
     *
     * @param menu Menu Name
     */
    public String[] getPath(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Home");
    }

    /**
     * Add path for given Menu
     *
     * @param menu Menu Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        throw new UnsupportedOperationException("Operation not supported for Home");
    }

    /**
     * Get available menu option
     *
     * @param menu Menu Name
     */
    public String[] getAvailableOptions(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Home");
    }

    /**
     * Add available menu options
     *
     * @param menu Menu Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        throw new UnsupportedOperationException("Operation not supported for Home");
    }

    /**
     * Get App Grid Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mHomeConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mHomeConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate Home Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mHomeConfigMap.get(configName) != null);
    }

    /**
     * Validate Home Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mHomeConfigMap.get(configName) == null) {
            return false;
        }
        return (mHomeConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Menu Options
     *
     * @param menu Menu Name
     */
    public boolean isValidOption(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Home Application");
    }

    /**
     * Validate Menu Path
     *
     * @param menu Menu Name
     */
    public boolean isValidPath(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Home Application");
    }

    /** Load default configuration for App Grid application */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default App Grid Application Config
        loadDefaultHomeConfig(mApplicationConfigMap);
        // Default App Grid view Config
        loadDefaultHomeViewConfig(mHomeConfigMap);
    }

    private void loadDefaultHomeConfig(Map<String, String> mApplicationConfigMap) {
        // Add default App Grid package
        mApplicationConfigMap.put(AutoConfigConstants.HOME_PACKAGE, HOME_PACKAGE);
    }

    private void loadDefaultHomeViewConfig(Map<String, AutoConfiguration> mHomeConfigMap) {
        AutoConfiguration homeViewConfiguration = new AutoConfiguration();
        homeViewConfiguration.addResource(
                AutoConfigConstants.TOP_CARD,
                new AutoConfigResource(AutoConfigConstants.RESOURCE_ID, "top_card", HOME_PACKAGE));
        homeViewConfiguration.addResource(
                AutoConfigConstants.BOTTOM_CARD,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "bottom_card", HOME_PACKAGE));
        homeViewConfiguration.addResource(
                AutoConfigConstants.MAP_CARD,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "scene_card", HOME_PACKAGE));
        mHomeConfigMap.put(AutoConfigConstants.HOME_VIEW, homeViewConfiguration);
    }
}
