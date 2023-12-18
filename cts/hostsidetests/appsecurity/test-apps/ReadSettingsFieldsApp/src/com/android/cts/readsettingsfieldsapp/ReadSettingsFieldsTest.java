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

package com.android.cts.readsettingsfieldsapp;

import android.content.ContentResolver;
import android.provider.Settings;
import android.test.AndroidTestCase;
import android.util.ArraySet;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;

public class ReadSettingsFieldsTest extends AndroidTestCase {

    private final String[] settingsKeysWithMaxTargetSdk = {"mobile_data"};

    /** Test public keys are readable with annotation */
    public void testSecureNonHiddenSettingsKeysAreReadable() {
        testNonHiddenSettingsKeysAreReadable(Settings.Secure.class);
    }

    public void testSystemNonHiddenSettingsKeysAreReadable() {
        testNonHiddenSettingsKeysAreReadable(Settings.System.class);
    }

    public void testGlobalNonHiddenSettingsKeysAreReadable() {
        testNonHiddenSettingsKeysAreReadable(Settings.Global.class);
    }

    private <T extends Settings.NameValueTable> void testNonHiddenSettingsKeysAreReadable(
            Class<T> settingsClass) {
        for (String key : getNonHiddenSettingsKeys(settingsClass)) {
            try {
                callGetStringMethod(settingsClass, key);
            } catch (SecurityException ex) {
                if (isSettingsDeprecated(ex)) {
                    continue;
                }
                // Skip checking for keys with maxTargetSdk because they might not be readable
                if (Arrays.asList(settingsKeysWithMaxTargetSdk).contains(key)) {
                    continue;
                }
                fail("Reading public " + settingsClass.getSimpleName() + " settings key <" + key
                        + "> should not raise exception! "
                        + "Did you forget to add @Readable annotation?\n" + ex.getMessage());
            }
        }
    }

    private <T extends Settings.NameValueTable> void callGetStringMethod(Class<T> settingsClass,
            String key) throws SecurityException {
        try {
            Method getStringMethod = settingsClass.getMethod("getString",
                    ContentResolver.class, String.class);
            getStringMethod.invoke(null, getContext().getContentResolver(), key);
        } catch (NoSuchMethodException | IllegalAccessException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            throw new SecurityException(e.getCause());
        }
    }

    private <T> ArraySet<String> getNonHiddenSettingsKeys(Class<T> settingsClass) {
        final ArraySet<String> publicSettingsKeys = new ArraySet<>();
        final Field[] allFields = settingsClass.getDeclaredFields();
        try {
            for (int i = 0; i < allFields.length; i++) {
                final Field field = allFields[i];
                if (field.getType().equals(String.class)) {
                    final Object value = field.get(settingsClass);
                    if (value.getClass().equals(String.class)) {
                        publicSettingsKeys.add((String) value);
                    }
                }
            }
        } catch (IllegalAccessException ignored) {
        }
        return publicSettingsKeys;
    }

    private boolean isSettingsDeprecated(SecurityException ex) {
        return ex.getMessage().contains("is deprecated and no longer accessible");
    }

    /** Test hidden keys are readable with annotation */
    public void testSecureSomeHiddenSettingsKeysAreReadable() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.Secure.class);
        final String[] hiddenSettingsKeys = {"adaptive_sleep", "bugreport_in_power_menu",
                "input_methods_subtype_history"};
        testHiddenSettingsKeysReadable(Settings.Secure.class, publicSettingsKeys,
                hiddenSettingsKeys);
    }

    public void testSystemSomeHiddenSettingsKeysAreReadable() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.System.class);
        final String[] hiddenSettingsKeys = {"advanced_settings", "system_locales",
                "display_color_mode", "min_refresh_rate"};
        testHiddenSettingsKeysReadable(Settings.System.class, publicSettingsKeys,
                hiddenSettingsKeys);
    }

    public void testGlobalSomeHiddenSettingsKeysAreReadable() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.Secure.class);
        final String[] hiddenSettingsKeys = {"add_users_when_locked",
                "enable_accessibility_global_gesture_enabled"};
        testHiddenSettingsKeysReadable(Settings.Global.class, publicSettingsKeys,
                hiddenSettingsKeys);
    }

    private <T extends Settings.NameValueTable> void testHiddenSettingsKeysReadable(
            Class<T> settingsClass, ArraySet<String> publicKeys, String[] targetKeys) {
        for (String key : targetKeys) {
            // Verify that the hidden keys are not visible to the test app
            assertFalse("Settings key <" + key + "> should not be visible",
                    publicKeys.contains(key));
            try {
                // Verify that the hidden keys can still be read
                callGetStringMethod(settingsClass, key);
            } catch (SecurityException ex) {
                fail("Reading hidden " + settingsClass.getSimpleName() + " settings key <" + key
                        + "> should not raise!");
            }
        }
    }

    /** Test hidden keys are not readable without annotation */
    public void testSecureHiddenSettingsKeysNotReadableWithoutAnnotation() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.Secure.class);
        final String[] hiddenSettingsKeys = {"camera_autorotate",
                "location_time_zone_detection_enabled"};
        testHiddenSettingsKeysNotReadableWithoutAnnotation(Settings.Secure.class,
                publicSettingsKeys, hiddenSettingsKeys);
    }

    public void testSystemHiddenSettingsKeysNotReadableWithoutAnnotation() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.System.class);
        final String[] hiddenSettingsKeys = {"display_color_mode_vendor_hint"};
        testHiddenSettingsKeysNotReadableWithoutAnnotation(Settings.System.class,
                publicSettingsKeys, hiddenSettingsKeys);
    }

    public void testGlobalHiddenSettingsKeysNotReadableWithoutAnnotation() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.Global.class);
        final String[] hiddenSettingsKeys = {"restricted_networking_mode",
                "people_space_conversation_type"};
        testHiddenSettingsKeysNotReadableWithoutAnnotation(Settings.Global.class,
                publicSettingsKeys, hiddenSettingsKeys);
    }

    // test the cases that hidden keys are marked with readable annotation but access should be
    // protected by additional permission check.
    public void testGlobalHiddenSettingsKeyNotReadableWithoutPermissions() {
        final String[] hiddenSettingsKeysRequiresPermissions = {"multi_sim_data_call"};
        for (String key : hiddenSettingsKeysRequiresPermissions) {
            try {
                // Verify that the hidden keys can't be accessed due to lack of permissions.
                callGetStringMethod(Settings.Global.class, key);
            } catch (SecurityException ex) {
                assertTrue(ex.getMessage().contains("permission"));
                continue;
            }
            fail("Reading hidden " + Settings.Global.class.getSimpleName() + " settings key <" + key
                    + "> should be protected with permission!");
        }
    }

    private <T extends Settings.NameValueTable>
    void testHiddenSettingsKeysNotReadableWithoutAnnotation(
            Class<T> settingsClass, ArraySet<String> publicKeys, String[] targetKeys) {
        for (String key : targetKeys) {
            // Verify that the hidden keys are not visible to the test app
            assertFalse("Settings key <" + key + "> should not be visible",
                    publicKeys.contains(key));
            try {
                // Verify that the hidden keys cannot be read
                callGetStringMethod(settingsClass, key);
                fail("Reading hidden " + settingsClass.getSimpleName() + " settings key <" + key
                        + "> should raise!");
            } catch (SecurityException ex) {
                assertTrue(ex.getMessage().contains(
                        "Settings key: <" + key + "> is not readable."));
            }
        }
    }

    /** Test hidden keys are readable if the app is test only, even without annotation */
    public void testSecureHiddenSettingsKeysReadableWithoutAnnotation() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.Secure.class);
        final String[] hiddenSettingsKeys = {"camera_autorotate",
                "location_time_zone_detection_enabled"};
        testHiddenSettingsKeysReadable(Settings.Secure.class, publicSettingsKeys,
                hiddenSettingsKeys);
    }

    public void testSystemHiddenSettingsKeysReadableWithoutAnnotation() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.System.class);
        final String[] hiddenSettingsKeys = {"display_color_mode_vendor_hint"};
        testHiddenSettingsKeysReadable(Settings.System.class, publicSettingsKeys,
                hiddenSettingsKeys);
    }

    public void testGlobalHiddenSettingsKeysReadableWithoutAnnotation() {
        final ArraySet<String> publicSettingsKeys = getNonHiddenSettingsKeys(Settings.Global.class);
        final String[] hiddenSettingsKeys = {"restricted_networking_mode",
                "people_space_conversation_type"};
        testHiddenSettingsKeysReadable(Settings.Global.class, publicSettingsKeys,
                hiddenSettingsKeys);
    }
}

