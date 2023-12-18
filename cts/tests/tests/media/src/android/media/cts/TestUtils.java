/*
 * Copyright 2018 The Android Open Source Project
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

package android.media.cts;

import static android.content.pm.PackageManager.MATCH_APEX;

import static org.junit.Assume.assumeTrue;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Assert;
import org.junit.AssumptionViolatedException;

import java.util.Objects;

/**
 * Utilities for tests.
 */
public final class TestUtils {
    private static String TAG = "TestUtils";
    private static final int WAIT_TIME_MS = 1000;
    private static final int WAIT_SERVICE_TIME_MS = 5000;

    /**
     * Compares contents of two bundles.
     *
     * @param a a bundle
     * @param b another bundle
     * @return {@code true} if two bundles are the same. {@code false} otherwise. This may be
     *     incorrect if any bundle contains a bundle.
     */
    public static boolean equals(Bundle a, Bundle b) {
        if (a == b) {
            return true;
        }
        if (a == null || b == null) {
            return false;
        }
        if (!a.keySet().containsAll(b.keySet())
                || !b.keySet().containsAll(a.keySet())) {
            return false;
        }
        for (String key : a.keySet()) {
            if (!Objects.equals(a.get(key), b.get(key))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Checks {@code module} is at least {@code minVersion}
     *
     * The tests are skipped by throwing a {@link AssumptionViolatedException}.  CTS test runners
     * will report this as a {@code ASSUMPTION_FAILED}.
     *
     * @param module     the apex module name
     * @param minVersion the minimum version
     * @throws AssumptionViolatedException if module version < minVersion
     */
    static void assumeMainlineModuleAtLeast(String module, long minVersion) {
        try {
            long actualVersion = getModuleVersion(module);
            assumeTrue("Assume  module  " + module + " version " + actualVersion + " < minVersion"
                    + minVersion, actualVersion >= minVersion);
        } catch (PackageManager.NameNotFoundException e) {
            Assert.fail(e.getMessage());
        }
    }

    /**
     * Checks if {@code module} is < {@code minVersion}
     *
     * <p>
     * {@link AssumptionViolatedException} is not handled properly by {@code JUnit3} so just return
     * the test
     * early instead.
     *
     * @param module     the apex module name
     * @param minVersion the minimum version
     * @deprecated convert test to JUnit4 and use
     * {@link #assumeMainlineModuleAtLeast(String, long)} instead.
     */
    @Deprecated
    static boolean skipTestIfMainlineLessThan(String module, long minVersion) {
        try {
            long actualVersion = getModuleVersion(module);
            if (actualVersion < minVersion) {
                Log.i(TAG, "Skipping test because Module  " + module + " minVersion " + minVersion
                        + " > "
                        + minVersion
                );
                return true;
            } else {
                return false;
            }
        } catch (PackageManager.NameNotFoundException e) {
            Assert.fail(e.getMessage());
            return false;
        }
    }

    private static long getModuleVersion(String module)
            throws PackageManager.NameNotFoundException {
        Context context = ApplicationProvider.getApplicationContext();
        PackageInfo info;
        info = context.getPackageManager().getPackageInfo(module,
                MATCH_APEX);
        return info.getLongVersionCode();
    }


    private TestUtils() {
    }

    public static class Monitor {
        private int mNumSignal;

        public synchronized void reset() {
            mNumSignal = 0;
        }

        public synchronized void signal() {
            mNumSignal++;
            notifyAll();
        }

        public synchronized boolean waitForSignal() throws InterruptedException {
            return waitForCountedSignals(1) > 0;
        }

        public synchronized int waitForCountedSignals(int targetCount) throws InterruptedException {
            while (mNumSignal < targetCount) {
                wait();
            }
            return mNumSignal;
        }

        public synchronized boolean waitForSignal(long timeoutMs) throws InterruptedException {
            return waitForCountedSignals(1, timeoutMs) > 0;
        }

        public synchronized int waitForCountedSignals(int targetCount, long timeoutMs)
                throws InterruptedException {
            if (timeoutMs == 0) {
                return waitForCountedSignals(targetCount);
            }
            long deadline = System.currentTimeMillis() + timeoutMs;
            while (mNumSignal < targetCount) {
                long delay = deadline - System.currentTimeMillis();
                if (delay <= 0) {
                    break;
                }
                wait(delay);
            }
            return mNumSignal;
        }

        public synchronized boolean isSignalled() {
            return mNumSignal >= 1;
        }

        public synchronized int getNumSignal() {
            return mNumSignal;
        }
    }
}
