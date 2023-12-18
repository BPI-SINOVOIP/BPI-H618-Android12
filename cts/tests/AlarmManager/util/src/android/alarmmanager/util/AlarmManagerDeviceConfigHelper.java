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

package android.alarmmanager.util;

import static org.junit.Assert.assertTrue;

import android.provider.DeviceConfig;
import android.util.Log;

import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.SystemUtil;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class AlarmManagerDeviceConfigHelper {
    private static final String TAG = AlarmManagerDeviceConfigHelper.class.getSimpleName();

    private static final long UPDATE_TIMEOUT = 30_000;

    private volatile Map<String, String> mCommittedMap;
    private final Map<String, String> mInitialPropertiesMap;
    private final Map<String, String> mPropertiesToSetMap;

    public AlarmManagerDeviceConfigHelper() {
        mPropertiesToSetMap = new HashMap<>();
        DeviceConfig.Properties initialProperties = null;
        try {
            initialProperties = SystemUtil.callWithShellPermissionIdentity(
                    () -> DeviceConfig.getProperties(DeviceConfig.NAMESPACE_ALARM_MANAGER));
        } catch (Exception e) {
            Log.e(TAG, "Unexpected exception while fetching device_config properties", e);
        }
        if (initialProperties != null) {
            for (String key : initialProperties.getKeyset()) {
                mPropertiesToSetMap.put(key, initialProperties.getString(key, null));
            }
            mCommittedMap = mInitialPropertiesMap = Collections.unmodifiableMap(
                    new HashMap<>(mPropertiesToSetMap));
        } else {
            mCommittedMap = mInitialPropertiesMap =  Collections.emptyMap();
        }
    }

    public AlarmManagerDeviceConfigHelper with(String key, long value) {
        return with(key, Long.toString(value));
    }

    public AlarmManagerDeviceConfigHelper with(String key, int value) {
        return with(key, Integer.toString(value));
    }

    public AlarmManagerDeviceConfigHelper with(String key, boolean value) {
        return with(key, Boolean.toString(value));
    }

    public AlarmManagerDeviceConfigHelper with(String key, String value) {
        mPropertiesToSetMap.put(key, value);
        return this;
    }

    public AlarmManagerDeviceConfigHelper without(String key) {
        mPropertiesToSetMap.remove(key);
        return this;
    }

    private static int getCurrentConfigVersion() {
        final String output = SystemUtil.runShellCommand("cmd alarm get-config-version").trim();
        return Integer.parseInt(output);
    }

    public static void commitAndAwaitPropagation(DeviceConfig.Properties propertiesToSet) {
        final int currentVersion = getCurrentConfigVersion();
        SystemUtil.runWithShellPermissionIdentity(
                () -> assertTrue(DeviceConfig.setProperties(propertiesToSet)));
        PollingCheck.waitFor(UPDATE_TIMEOUT, () -> (getCurrentConfigVersion() > currentVersion),
                "Could not update config within " + UPDATE_TIMEOUT + "ms. Current version: "
                        + currentVersion);
    }

    public void commitAndAwaitPropagation() {
        if (mPropertiesToSetMap.equals(mCommittedMap)) {
            // This will not cause any change, and will not bump up the config version.
            return;
        }
        commitAndAwaitPropagation(
                new DeviceConfig.Properties(DeviceConfig.NAMESPACE_ALARM_MANAGER,
                        mPropertiesToSetMap));
        mCommittedMap = Collections.unmodifiableMap(new HashMap<>(mPropertiesToSetMap));
    }

    public void restoreAll() {
        if (mCommittedMap.equals(mInitialPropertiesMap)) {
            // This will not cause any change, and will not bump up the config version.
            return;
        }
        commitAndAwaitPropagation(new DeviceConfig.Properties(DeviceConfig.NAMESPACE_ALARM_MANAGER,
                mInitialPropertiesMap));
        mCommittedMap = Collections.emptyMap();
    }
}
