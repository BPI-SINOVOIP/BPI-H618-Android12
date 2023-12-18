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

/** Configuration for Lock Screen */
public class AutoLockScreenConfigUtility implements IAutoConfigUtility {

    private static final String LOG_TAG = AutoLockScreenConfigUtility.class.getSimpleName();

    // Lock Screen Config
    private static final String LOCK_SCREEN_PACKAGE = "com.android.systemui";

    // Lock Screen Map
    private Map<String, AutoConfiguration> mLockScreenConfigMap;

    private static AutoLockScreenConfigUtility sAutoLockScreenConfigInstance = null;

    private AutoLockScreenConfigUtility() {
        // Initialize Lock Screen Config Map
        mLockScreenConfigMap = new HashMap<String, AutoConfiguration>();
    }

    /** Get instance of Auto Lock Screen Utility */
    public static IAutoConfigUtility getInstance() {
        if (sAutoLockScreenConfigInstance == null) {
            sAutoLockScreenConfigInstance = new AutoLockScreenConfigUtility();
        }
        return sAutoLockScreenConfigInstance;
    }

    /**
     * Get Path for given menu
     *
     * @param menu Menu Name
     */
    public String[] getPath(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Lock Screen");
    }

    /**
     * Add path for given Menu
     *
     * @param menu Menu Name
     * @param path Path
     */
    public void addPath(String menu, String[] path) {
        throw new UnsupportedOperationException("Operation not supported for Lock Screen");
    }

    /**
     * Get available menu option
     *
     * @param menu Menu Name
     */
    public String[] getAvailableOptions(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Lock Screen");
    }

    /**
     * Add available menu options
     *
     * @param menu Menu Name
     * @param options Available Options
     */
    public void addAvailableOptions(String menu, String[] options) {
        throw new UnsupportedOperationException("Operation not supported for Lock Screen");
    }

    /**
     * Get App Grid Config Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public AutoConfigResource getResourceConfiguration(String configName, String resourceName) {
        if (mLockScreenConfigMap.get(configName) == null) {
            // Unknown Configuration
            return null;
        }
        return mLockScreenConfigMap.get(configName).getResource(resourceName);
    }

    /**
     * Validate Home Configuration
     *
     * @param configName Configuration Name
     */
    public boolean isValidConfiguration(String configName) {
        return (mLockScreenConfigMap.get(configName) != null);
    }

    /**
     * Validate Home Configuration Resource
     *
     * @param configName Configuration Name
     * @param resourceName Resource Name
     */
    public boolean isValidResource(String configName, String resourceName) {
        if (mLockScreenConfigMap.get(configName) == null) {
            return false;
        }
        return (mLockScreenConfigMap.get(configName).getResource(resourceName) != null);
    }

    /**
     * Validate Menu Options
     *
     * @param menu Menu Name
     */
    public boolean isValidOption(String menu) {
        throw new UnsupportedOperationException(
                "Operation not supported for Lock Screen Application");
    }

    /**
     * Validate Menu Path
     *
     * @param menu Menu Name
     */
    public boolean isValidPath(String menu) {
        throw new UnsupportedOperationException("Operation not supported for Lock Screen");
    }

    /** Load default configuration for Lock Screen */
    public void loadDefaultConfig(Map<String, String> mApplicationConfigMap) {
        // Default Lock Screen Config
        loadDefaultLockScreenConfig(mApplicationConfigMap);
        // Default Lock Screen view Config
        loadDefaultLockScreenViewConfig(mLockScreenConfigMap);
    }

    private void loadDefaultLockScreenConfig(Map<String, String> mApplicationConfigMap) {
        // Add default App Grid package
        mApplicationConfigMap.put(AutoConfigConstants.LOCK_SCREEN_PACKAGE, LOCK_SCREEN_PACKAGE);
    }

    private void loadDefaultLockScreenViewConfig(
            Map<String, AutoConfiguration> mLockScreenConfigMap) {
        AutoConfiguration lockScreenViewConfiguration = new AutoConfiguration();
        lockScreenViewConfiguration.addResource(
                AutoConfigConstants.PIN_PAD,
                new AutoConfigResource(
                        AutoConfigConstants.PIN_PAD, "keyguard_pin_view", LOCK_SCREEN_PACKAGE));
        lockScreenViewConfiguration.addResource(
                AutoConfigConstants.ENTER_KEY,
                new AutoConfigResource(
                        AutoConfigConstants.RESOURCE_ID, "key_enter", LOCK_SCREEN_PACKAGE));
        mLockScreenConfigMap.put(AutoConfigConstants.LOCK_SCREEN_VIEW, lockScreenViewConfiguration);
    }
}
