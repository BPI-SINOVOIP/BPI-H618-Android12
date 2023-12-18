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

/** Configuration Class for mapping Resources to Automotive Device UI */
public class AutoConfiguration {
    // Configuration Map
    private Map<String, AutoConfigResource> configMap;

    // Constructor
    public AutoConfiguration() {
        this.configMap = new HashMap<String, AutoConfigResource>();
    }

    /** Add the resource to map */
    public void addResource(String key, AutoConfigResource resource) {
        this.configMap.put(key, resource);
    }

    /** Get mapped Resource */
    public AutoConfigResource getResource(String key) {
        return this.configMap.get(key);
    }
}
