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

package android.app.time.cts.shell;

/** Constants for interacting with the device_config service. */
public final class DeviceConfigKeys {

    /**
     * The DeviceConfig namespace used for the time_detector, time_zone_detector and
     * location_time_zone_manager.
     */
    public static final String NAMESPACE_SYSTEM_TIME = "system_time";

    private DeviceConfigKeys() {
        // No need to instantiate.
    }

    /**
     * Keys and values associated with the location_time_zone_manager.
     * See also {@link LocationTimeZoneManagerShellHelper}.
     */
    final class LocationTimeZoneManager {

        private LocationTimeZoneManager() {
            // No need to instantiate.
        }

        /**
         * The key for the server flag that can override the device config for whether the primary
         * location time zone provider is enabled, disabled, or (for testing) in simulation mode.
         */
        static final String KEY_PRIMARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE =
                "primary_location_time_zone_provider_mode_override";

        /**
         * The key for the server flag that can override the device config for whether the secondary
         * location time zone provider is enabled or disabled, or (for testing) in simulation mode.
         */
        static final String KEY_SECONDARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE =
                "secondary_location_time_zone_provider_mode_override";

        /**
         * The "simulated" provider mode.
         * For use with {@link #KEY_PRIMARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE} and {@link
         * #KEY_SECONDARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE}.
         */
        static final String PROVIDER_MODE_SIMULATED = "simulated";

        /**
         * The "disabled" provider mode (equivalent to there being no provider configured).
         * For use with {@link #KEY_PRIMARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE} and {@link
         * #KEY_SECONDARY_LOCATION_TIME_ZONE_PROVIDER_MODE_OVERRIDE}.
         */
        static final String PROVIDER_MODE_DISABLED = "disabled";

        /**
         * The key for the server flag that can override the device config for the package name of
         * the primary provider (when enabled).
         */
        static final String KEY_PRIMARY_LOCATION_TIME_ZONE_PROVIDER_PACKAGE_NAME_OVERRIDE =
                "primary_location_time_zone_provider_package_name_override";

        /**
         * The key for the server flag that can override the device config for the package name of
         * the secondary provider (when enabled).
         */
        static final String KEY_SECONDARY_LOCATION_TIME_ZONE_PROVIDER_PACKAGE_NAME_OVERRIDE =
                "secondary_location_time_zone_provider_package_name_override";
    }

    /**
     * Keys and values associated with the time_detector. See also {@link
     * TimeZoneDetectorShellHelper}.
     */
    public final class TimeDetector {

        private TimeDetector() {
            // No need to instantiate.
        }

        /**
         * See {@link
         * com.android.server.timedetector.ServerFlags#KEY_TIME_DETECTOR_ORIGIN_PRIORITIES_OVERRIDE}
         */
        public static final String KEY_TIME_DETECTOR_ORIGIN_PRIORITIES_OVERRIDE =
                "time_detector_origin_priorities_override";

        /**
         * See {@link com.android.server.timedetector.TimeDetectorStrategy#ORIGIN_NETWORK}.
         */
        public static final String ORIGIN_NETWORK = "network";

        /**
         * See {@link com.android.server.timedetector.TimeDetectorStrategy#ORIGIN_EXTERNAL}.
         */
        public static final String ORIGIN_EXTERNAL = "external";
    }
}
