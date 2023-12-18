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

package android.devicepolicy.cts;

import static org.junit.Assert.assertNull;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.LauncherApps;
import android.os.Process;

import androidx.test.core.app.ApplicationProvider;

import com.android.bedstead.harrier.BedsteadJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(BedsteadJUnit4.class)
public final class LauncherAppsTests {
    private final Context sContext = ApplicationProvider.getApplicationContext();
    private final LauncherApps sLauncherApps = sContext.getSystemService(LauncherApps.class);

    @Test
    public void testResolveInvalidActivity_doesNotCrash() {
        final Intent intent = new Intent();
        intent.setComponent(new ComponentName("invalidPackage", "invalidClass"));

        // Test that resolving invalid intent does not crash launcher
        assertNull(sLauncherApps.resolveActivity(intent, Process.myUserHandle()));
    }
}
