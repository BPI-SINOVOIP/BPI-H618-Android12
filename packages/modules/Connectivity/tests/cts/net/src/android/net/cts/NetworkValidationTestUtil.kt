/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.net.cts

import android.Manifest
import android.net.util.NetworkStackUtils
import android.provider.DeviceConfig
import com.android.testutils.runAsShell

/**
 * Collection of utility methods for configuring network validation.
 */
internal object NetworkValidationTestUtil {

    /**
     * Clear the test network validation URLs.
     */
    @JvmStatic fun clearValidationTestUrlsDeviceConfig() {
        setHttpsUrlDeviceConfig(null)
        setHttpUrlDeviceConfig(null)
        setUrlExpirationDeviceConfig(null)
    }

    /**
     * Set the test validation HTTPS URL.
     *
     * @see NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTPS_URL
     */
    @JvmStatic fun setHttpsUrlDeviceConfig(url: String?) =
            setConfig(NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTPS_URL, url)

    /**
     * Set the test validation HTTP URL.
     *
     * @see NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTP_URL
     */
    @JvmStatic fun setHttpUrlDeviceConfig(url: String?) =
            setConfig(NetworkStackUtils.TEST_CAPTIVE_PORTAL_HTTP_URL, url)

    /**
     * Set the test validation URL expiration.
     *
     * @see NetworkStackUtils.TEST_URL_EXPIRATION_TIME
     */
    @JvmStatic fun setUrlExpirationDeviceConfig(timestamp: Long?) =
            setConfig(NetworkStackUtils.TEST_URL_EXPIRATION_TIME, timestamp?.toString())

    private fun setConfig(configKey: String, value: String?) {
        runAsShell(Manifest.permission.WRITE_DEVICE_CONFIG) {
            DeviceConfig.setProperty(
                    DeviceConfig.NAMESPACE_CONNECTIVITY, configKey, value, false /* makeDefault */)
        }
    }
}