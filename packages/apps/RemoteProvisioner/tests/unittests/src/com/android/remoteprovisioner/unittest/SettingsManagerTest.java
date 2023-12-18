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

package com.android.remoteprovisioner.unittest;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.runner.AndroidJUnit4;

import com.android.remoteprovisioner.SettingsManager;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.time.Duration;

@RunWith(AndroidJUnit4.class)
public class SettingsManagerTest {

    private static Context sContext;

    @BeforeClass
    public static void init() {
        sContext = ApplicationProvider.getApplicationContext();
    }

    @Before
    public void setUp() {
        SettingsManager.clearPreferences(sContext);
    }

    @After
    public void tearDown() {
        SettingsManager.clearPreferences(sContext);
    }

    @Test
    public void testCheckDefaults() throws Exception {
        assertEquals(SettingsManager.EXTRA_SIGNED_KEYS_AVAILABLE_DEFAULT,
                     SettingsManager.getExtraSignedKeysAvailable(sContext));
        assertEquals(SettingsManager.EXPIRING_BY_MS_DEFAULT,
                     SettingsManager.getExpiringBy(sContext).toMillis());
        assertEquals(SettingsManager.URL_DEFAULT,
                     SettingsManager.getUrl(sContext));
        assertEquals(0, SettingsManager.getFailureCounter(sContext));
    }

    @Test
    public void testCheckIdSettings() throws Exception {
        int defaultRandom = SettingsManager.getId(sContext);
        assertTrue("Default ID out of bounds.",
                defaultRandom < SettingsManager.ID_UPPER_BOUND && defaultRandom >= 0);
        SettingsManager.generateAndSetId(sContext);
        int setId = SettingsManager.getId(sContext);
        assertTrue("Stored ID out of bounds.",
                setId < SettingsManager.ID_UPPER_BOUND && setId >= 0);
        SettingsManager.generateAndSetId(sContext);
        assertEquals("ID should not be updated by a repeated call",
                     setId, SettingsManager.getId(sContext));
    }

    @Test
    public void testSetDeviceConfig() {
        int extraKeys = 12;
        Duration expiringBy = Duration.ofMillis(1000);
        String url = "https://www.remoteprovisionalot";
        assertTrue("Method did not return true on write.",
                   SettingsManager.setDeviceConfig(sContext, extraKeys, expiringBy, url));
        assertEquals(extraKeys, SettingsManager.getExtraSignedKeysAvailable(sContext));
        assertEquals(expiringBy.toMillis(), SettingsManager.getExpiringBy(sContext).toMillis());
        assertEquals(url, SettingsManager.getUrl(sContext));
    }

    @Test
    public void testFailureCounter() {
        assertEquals(1, SettingsManager.incrementFailureCounter(sContext));
        assertEquals(1, SettingsManager.getFailureCounter(sContext));
        for (int i = 1; i < 10; i++) {
            assertEquals(i + 1, SettingsManager.incrementFailureCounter(sContext));
        }
        SettingsManager.clearFailureCounter(sContext);
        assertEquals(0, SettingsManager.getFailureCounter(sContext));
        SettingsManager.incrementFailureCounter(sContext);
        assertEquals(1, SettingsManager.getFailureCounter(sContext));
    }
}
