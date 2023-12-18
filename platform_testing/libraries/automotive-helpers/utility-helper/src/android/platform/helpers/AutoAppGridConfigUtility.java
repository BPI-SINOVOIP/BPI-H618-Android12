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

/** Configuration for App Grid */
public class AutoAppGridConfigUtility implements IAutoConfigUtility {

    private static final String LOG_TAG = AutoAppGridConfigUtility.class.getSimpleName();

    // App Grid Config
    private static final String APP_GRID_PACKAGE = "com.android.car.carlauncher";
    private static final String APP_GRID_ACTIVITY = "com.android.car.carlauncher/.AppGridActivity";
    private static final String OPEN_APP_GRID_COMMAND =
            "am start -n com.android.car.carlauncher/.AppGridActivity";

    // Config Map
    private Map<String, AutoConfiguration> mAppGridConfigMap;

    private static AutoAppGridConfigUtility sAutoAppGridConfigInstance = null;

    private AutoAppGridConfigUtility() {
        // Initialize App Grid Config Map
        mAppGridConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get instance of Auto App Grid Utility */
    public static IAutoConfigUtility getInstance() {
        if (sAutoAppGridConfigInstance == null) {
            sAutoAppGridConfigInstance = new AutoAppGridConfigUtility();
        }
        return sAutoAppGridConfigInstance;
    }

    /**
     * Get Path for given menu
     *
     * @param menu Menu Name
     */
    public String[] getPath(String menu) {
        throw new UnsupportedOperationException("Operation not supported for App Grid");
    }

    /**
     * Add path for given Menu
     *
     * @param menu Menu Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        throw new UnsupportedOperationException("Operation not supported for App Grid");
    }

    /**
     * Get available menu option
     *
     * @param menu Menu Name
     */
    public String[] getAvailableOptions(String menu) {
        throw new UnsupportedOperationException("Operation not supported for App Grid");
    }

    /**
     * Add available menu options
     *
     * @param menu Menu Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        throw new UnsupportedOperationException("Operation not supported for App Grid");
    }

    /**
     * Get App Grid Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mAppGridConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mAppGridConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate App Grid Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mAppGridConfigMap.get(configName) != null);
    }

    /**
     * Validate App Grid Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mAppGridConfigMap.get(configName) == null) {
            return false;
        }
        return (mAppGridConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Menu Options
     *
     * @param menu Menu Name
     */
    public boolean isValidOption(String menu) {
        throw new UnsupportedOperationException("Operation not supported for App Grid Application");
    }

    /**
     * Validate Menu Path
     *
     * @param menu Menu Name
     */
    public boolean isValidPath(String menu) {
        throw new UnsupportedOperationException("Operation not supported for App Grid Application");
    }

    /** Load default configuration for App Grid application */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default App Grid Application Config
        loadDefaultAppGridConfig(mApplicationConfigMap);
        // Default App Grid view Config
        loadDefaultAppGridViewConfig(mAppGridConfigMap);
    }

    private void loadDefaultAppGridConfig(Map<String, String> mApplicationConfigMap) {
        // Add default App Grid package
        mApplicationConfigMap.put(AutoConfigConstants.APP_GRID_PACKAGE, APP_GRID_PACKAGE);
        // Add default App Grid activity
        mApplicationConfigMap.put(AutoConfigConstants.APP_GRID_ACTIVITY, APP_GRID_ACTIVITY);
        // Add default command to open app grid
        mApplicationConfigMap.put(AutoConfigConstants.OPEN_APP_GRID_COMMAND, OPEN_APP_GRID_COMMAND);
    }

    private void loadDefaultAppGridViewConfig(
            Map<String, AutoConfiguration> mApplicationConfigMap) {
        AutoConfiguration appGridViewConfiguration = new AutoConfiguration();
        appGridViewConfiguration.addResource(
                AutoConfigConstants.APP_GRID_VIEW_ID,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "apps_grid", APP_GRID_PACKAGE));
        appGridViewConfiguration.addResource(
                AutoConfigConstants.APPLICATION_NAME,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "app_name", APP_GRID_PACKAGE));
        mAppGridConfigMap.put(AutoConfigConstants.APP_GRID_VIEW, appGridViewConfiguration);
    }
}
