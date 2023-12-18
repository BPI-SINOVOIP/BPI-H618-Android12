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

package android.car.cts;

import static com.android.compatibility.common.util.ShellUtils.runShellCommand;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.testng.Assert.assertThrows;

import android.car.Car;
import android.car.watchdog.CarWatchdogManager;
import android.car.watchdog.IoOveruseStats;
import android.car.watchdog.PerStateBytes;
import android.car.watchdog.ResourceOveruseStats;
import android.content.Context;
import android.os.Process;
import android.os.UserHandle;
import android.platform.test.annotations.AppModeFull;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import com.android.compatibility.common.util.PollingCheck;

import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.concurrent.atomic.AtomicReference;
@AppModeFull(reason = "Instant Apps cannot get car related permissions")
public class CarWatchdogManagerTest extends CarApiTestBase {
    private static final String TAG = CarWatchdogManagerTest.class.getSimpleName();
    private static final String CAR_WATCHDOG_SERVICE_NAME
            = "android.automotive.watchdog.ICarWatchdog/default";
    private static final long FIVE_HUNDRED_KILOBYTES = 1024 * 500;
    // Wait time to sync I/O stats from proc fs -> watchdog daemon -> CarService.
    private static final int STATS_SYNC_WAIT_MS = 3000;

    private Context mContext;
    private CarWatchdogManager mCarWatchdogManager;
    private File mFile;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        mFile = mContext.getFilesDir();
        mCarWatchdogManager = (CarWatchdogManager) getCar().getCarManager(Car.CAR_WATCHDOG_SERVICE);
    }

    @Test
    public void testGetResourceOveruseStats() throws Exception {
        String packageName = mContext.getPackageName();
        /*
         * Start a custom performance collection with a 1 second interval. This enables watchdog
         * daemon to read proc stats more frequently and reduces the test wait time.
         */
        runShellCommand(
                "dumpsys %s --start_perf "
                        + "--max_duration 600 --interval 1", CAR_WATCHDOG_SERVICE_NAME);
        /*
         * Reset the resource overuse stats before running the test. Clears any previous stats
         * saved by watchdog.
         */
        runShellCommand(
                "dumpsys %s --reset_resource_overuse_stats %s", CAR_WATCHDOG_SERVICE_NAME,
                packageName);
        long writtenBytes = writeToDisk(mFile, FIVE_HUNDRED_KILOBYTES);
        assertWithMessage("Failed to write data to dir '" + mFile.getAbsolutePath() + "'").that(
                writtenBytes).isGreaterThan(0L);
        AtomicReference<ResourceOveruseStats> stats = new AtomicReference<>();
        PollingCheck.waitFor(STATS_SYNC_WAIT_MS, () -> {
            stats.set(mCarWatchdogManager.getResourceOveruseStats(
                    CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO,
                    CarWatchdogManager.STATS_PERIOD_CURRENT_DAY));
            return stats.get().getIoOveruseStats() != null;
        });
        /*
         * Stop the custom performance collection. Resets watchdog's I/O stat collection to default
         * interval.
         */
        runShellCommand("dumpsys %s --stop_perf", CAR_WATCHDOG_SERVICE_NAME);

        IoOveruseStats ioOveruseStats = stats.get().getIoOveruseStats();
        PerStateBytes remainingWriteBytes = ioOveruseStats.getRemainingWriteBytes();
        assertWithMessage("Package name").that(stats.get().getPackageName()).isEqualTo(packageName);
        assertWithMessage("Total bytes written to disk").that(
                ioOveruseStats.getTotalBytesWritten()).isAtLeast(FIVE_HUNDRED_KILOBYTES);
        assertWithMessage("Remaining write bytes").that(remainingWriteBytes).isNotNull();
        assertWithMessage("Remaining foreground write bytes").that(
                remainingWriteBytes.getForegroundModeBytes()).isGreaterThan(0);
        assertWithMessage("Remaining background write bytes").that(
                remainingWriteBytes.getBackgroundModeBytes()).isGreaterThan(0);
        assertWithMessage("Remaining garage mode write bytes").that(
                remainingWriteBytes.getGarageModeBytes()).isGreaterThan(0);
        assertWithMessage("Duration in seconds").that(
                ioOveruseStats.getDurationInSeconds()).isGreaterThan(0);
        assertWithMessage("Start time").that(ioOveruseStats.getStartTime()).isGreaterThan(0);
        assertWithMessage("Total overuse").that(ioOveruseStats.getTotalOveruses()).isEqualTo(0);
        assertWithMessage("Total times killed").that(
                ioOveruseStats.getTotalTimesKilled()).isEqualTo(0);
        assertWithMessage("Killable on overuse").that(
                ioOveruseStats.isKillableOnOveruse()).isTrue();
        assertWithMessage("User handle").that(stats.get().getUserHandle()).isEqualTo(
                UserHandle.getUserHandleForUid(Process.myUid()));
    }

    /**
     * Test that no exception is thrown when calling the addResourceOveruseListener and
     * removeResourceOveruseListener client APIs.
     *
     * <p>The actual notification handling and killing will
     * be tested with host side tests.
     */
    @Test
    public void testListenIoOveruse() {
        CarWatchdogManager.ResourceOveruseListener listener = resourceOveruseStats -> {
            // Do nothing
        };

        mCarWatchdogManager.addResourceOveruseListener(
                mContext.getMainExecutor(), CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO, listener);
        mCarWatchdogManager.removeResourceOveruseListener(listener);
    }

    @Test
    public void testThrowsExceptionOnNullResourceOveruseListener() {
        assertThrows(NullPointerException.class,
                () -> mCarWatchdogManager.addResourceOveruseListener(
                        mContext.getMainExecutor(), CarWatchdogManager.FLAG_RESOURCE_OVERUSE_IO,
                        null));
    }

    private static long writeToDisk(File dir, long size) throws Exception {
        if (!dir.exists()) {
            throw new FileNotFoundException(
                    "directory '" + dir.getAbsolutePath() + "' doesn't exist");
        }
        File uniqueFile = new File(dir, Long.toString(System.nanoTime()));
        try (FileOutputStream fos = new FileOutputStream(uniqueFile)) {
            Log.d(TAG, "Attempting to write " + size + " bytes");
            writeToFos(fos, size);
            fos.getFD().sync();
        }
        return size;
    }

    private static void writeToFos(FileOutputStream fos, long maxSize) throws IOException {
        while (maxSize != 0) {
            int writeSize = (int) Math.min(Integer.MAX_VALUE,
                    Math.min(Runtime.getRuntime().freeMemory(), maxSize));
            Log.i(TAG, "writeSize:" + writeSize);
            try {
                fos.write(new byte[writeSize]);
            } catch (InterruptedIOException e) {
                Thread.currentThread().interrupt();
                continue;
            }
            maxSize -= writeSize;
        }
    }
}
