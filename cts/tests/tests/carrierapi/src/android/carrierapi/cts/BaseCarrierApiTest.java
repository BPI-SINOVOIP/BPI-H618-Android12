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

package android.carrierapi.cts;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assume.assumeTrue;

import android.content.Context;
import android.telephony.TelephonyManager;

import androidx.test.InstrumentationRegistry;

import com.android.compatibility.common.util.FeatureUtil;

import org.junit.Before;

/**
 * Common test base to ensure uniform preconditions checking. This class will check for:
 *
 * <ol>
 *   <li>{@link android.content.pm.PackageManager#FEATURE_TELEPHONY}
 *   <li>A SIM that grants us carrier privileges is currently active in the device
 * </ol>
 *
 * Just inherit from this class when writing your test, then you are able to assume in the subclass
 * {@code Before} method that preconditions have all passed. The setup and test methods will not be
 * executed if preconditions are not met.
 */
public abstract class BaseCarrierApiTest {
    protected static final String NO_CARRIER_PRIVILEGES_FAILURE_MESSAGE =
            "This test requires a SIM card with carrier privilege rules on it.\n"
                    + "Visit https://source.android.com/devices/tech/config/uicc.html";

    protected Context getContext() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    private boolean mPreconditionsSatisfied = false;

    protected boolean werePreconditionsSatisfied() {
        return mPreconditionsSatisfied;
    }

    /**
     * Subclasses do NOT need to explicitly call or override this method. Per the JUnit docs, a
     * superclass {@code Before} method always executes before a subclass {@code Before} method.
     *
     * <p>If preconditions fail, neither the subclass {@code Before} method(s) nor the actual {@code
     * Test} method will execute, but {@code After} methods will still execute. If a subclass does
     * work in an {@code After} method, then it should first check {@link
     * #werePreconditionsSatisfied} and return early without doing any work if it's {@code false}.
     */
    @Before
    public void ensurePreconditionsMet() {
        mPreconditionsSatisfied = false;
        // Bail out if no cellular support.
        assumeTrue(
                "No cellular support, CarrierAPI."
                        + getClass().getSimpleName()
                        + " cases will be skipped",
                FeatureUtil.hasTelephony());
        // We must run with carrier privileges.
        assertWithMessage(NO_CARRIER_PRIVILEGES_FAILURE_MESSAGE)
                .that(getContext().getSystemService(TelephonyManager.class).hasCarrierPrivileges())
                .isTrue();
        mPreconditionsSatisfied = true;
    }
}
