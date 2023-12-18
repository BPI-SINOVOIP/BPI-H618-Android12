/*
 * Copyright 2020 The Android Open Source Project
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

package android.graphics.cts;

import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.pm.PackageManager;
import android.util.Log;

import androidx.test.InstrumentationRegistry;
import androidx.test.filters.SmallTest;
import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.PropertyUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test that feature flag android.software.opengles.deqp.level is present and that it has an
 * acceptable value.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class OpenGlEsDeqpLevelTest {

    private static final String TAG = OpenGlEsDeqpLevelTest.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final int MINIMUM_OPENGLES_DEQP_LEVEL = 0x07E40301; // Corresponds to 2020-03-01

    private PackageManager mPm;

    @Before
    public void setup() {
        mPm = InstrumentationRegistry.getTargetContext().getPackageManager();
    }

    @Test
    public void testOpenGlEsDeqpLevel() {
        assumeTrue(
                "Test only applies for vendor image with API level >= 31 (Android 12)",
                PropertyUtil.isVendorApiLevelNewerThan(30));
        if (DEBUG) {
            Log.d(TAG, "Checking whether " + PackageManager.FEATURE_OPENGLES_DEQP_LEVEL
                    + " has an acceptable value");
        }
        assertTrue("Feature " + PackageManager.FEATURE_OPENGLES_DEQP_LEVEL + " must be present "
                + "and have at least version " + MINIMUM_OPENGLES_DEQP_LEVEL,
                mPm.hasSystemFeature(PackageManager.FEATURE_OPENGLES_DEQP_LEVEL,
                        MINIMUM_OPENGLES_DEQP_LEVEL));
    }

}
