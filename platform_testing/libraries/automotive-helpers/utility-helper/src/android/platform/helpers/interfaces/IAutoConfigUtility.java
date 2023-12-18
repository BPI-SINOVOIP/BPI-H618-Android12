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

import java.util.Map;

public interface IAutoConfigUtility {
    /** This method is used loading default configuration. */
    void loadDefaultConfig(Map<String, String> mApplicationConfigMap);

    /** This method is used to get the resource configuration. */
    AutoConfigResource getResourceConfiguration(String configName, String resourceName);

    /** This method is used to get an array of available options for given menu. */
    String[] getAvailableOptions(String menu);

    /** This method is used to add an array of available options for given menu. */
    void addAvailableOptions(String menu, String[] options);

    /** This method is used to get path for given menu. */
    String[] getPath(String menu);

    /** This method is used to add path for given menu. */
    void addPath(String menu, String[] options);

    /** This method is used validate configuration. */
    boolean isValidConfiguration(String configName);

    /** This method is used validate configuration resource. */
    boolean isValidResource(String configName, String resourceName);

    /** This method is used validate options. */
    boolean isValidOption(String menu);

    /** This method is used validate path. */
    boolean isValidPath(String menu);
}
