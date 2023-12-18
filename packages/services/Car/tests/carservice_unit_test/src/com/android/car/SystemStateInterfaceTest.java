/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.car;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;

import com.android.car.internal.ICarServiceHelper;
import com.android.car.systeminterface.SystemStateInterface;
import com.android.settingslib.utils.ThreadUtils;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Unit tests for {@link SystemStateInterface}
 *
 * Run:
 * atest SystemStateInterfaceTest
 */
public class SystemStateInterfaceTest {
    private static final long HELPER_DELAY_MS = 50;

    @Mock private Context mMockContext;
    private SystemStateInterface.DefaultImpl mSystemStateInterface;
    private final ICarServiceHelper mTestHelperGood = new TestServiceHelperSucceeds();
    private final ICarServiceHelper mTestHelperFails = new TestServiceHelperFails();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mSystemStateInterface = new SystemStateInterface.DefaultImpl(mMockContext);
    }

    @Test
    public void testSleepWhenHelperSucceeds() {
        mSystemStateInterface.setCarServiceHelper(mTestHelperGood);
        assertThat(mSystemStateInterface.enterDeepSleep()).isTrue();
    }

    @Test
    public void testSleepWhenHelperFails() {
        mSystemStateInterface.setCarServiceHelper(mTestHelperFails);
        assertThat(mSystemStateInterface.enterDeepSleep()).isFalse();
    }

    @Test
    public void testSleepWithNullHelper() {
        mSystemStateInterface.setCarServiceHelper(null);
        assertThat(mSystemStateInterface.enterDeepSleep()).isFalse();
    }

    @Test
    public void testSleepWithDelayedHelperSucceeds() {
        assertThat(sleepWithDelayedHelper(mTestHelperGood)).isTrue();
    }

    @Test
    public void testSleepWithDelayedHelperFails() {
        assertThat(sleepWithDelayedHelper(mTestHelperFails)).isFalse();
    }

    @Test
    public void testSleepWithDelayedNullHelper() {
        assertThat(sleepWithDelayedHelper(null)).isFalse();
    }

    private static class TestServiceHelperSucceeds extends AbstractICarServiceHelperStub {
        @Override
        public int forceSuspend(int timeoutMs) {
            return 0; // Success
        }
    }
    private static class TestServiceHelperFails extends AbstractICarServiceHelperStub {
        @Override
        public int forceSuspend(int timeoutMs) {
            return 1; // Failure
        }
    }
    // Invoke enterDeepSleep() before setting the helper.
    // Return the result from enterDeepSleep().
    private boolean sleepWithDelayedHelper(ICarServiceHelper serviceHelper) {
        ThreadUtils.postOnBackgroundThread(() -> {
            // Provide the helper after a delay
            try {
                Thread.sleep(HELPER_DELAY_MS);
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt(); // Restore interrupted status
            }
            mSystemStateInterface.setCarServiceHelper(serviceHelper);
        });
        // Request deep sleep before the helper is provided
        return mSystemStateInterface.enterDeepSleep();
    }
}
