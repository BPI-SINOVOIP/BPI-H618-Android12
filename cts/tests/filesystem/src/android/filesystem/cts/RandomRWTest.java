/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.filesystem.cts;

import static androidx.test.InstrumentationRegistry.getContext;
import static androidx.test.InstrumentationRegistry.getInstrumentation;

import android.os.Environment;

import androidx.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.CddTest;
import com.android.compatibility.common.util.DeviceReportLog;

import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class RandomRWTest {
    private static final String DIR_RANDOM_WR = "RANDOM_WR";
    private static final String DIR_RANDOM_RD = "RANDOM_RD";
    private static final String REPORT_LOG_NAME = "CtsFileSystemTestCases";
    private static final double MIN_READ_MBPS;
    private static final double MIN_WRITE_MBPS;

    static {
        if (MediaPerformanceClassUtils.isRPerfClass()) {
            MIN_READ_MBPS = 25;
            MIN_WRITE_MBPS = 10;
        } else {
            // Performance class Build.VERSION_CODES.S and beyond
            MIN_READ_MBPS = 40;
            MIN_WRITE_MBPS = 10;
        }
    }

    @After
    public void tearDown() throws Exception {
        FileUtil.removeFileOrDir(getContext(), DIR_RANDOM_WR);
        FileUtil.removeFileOrDir(getContext(), DIR_RANDOM_RD);
    }

    @CddTest(requirement="8.2")
    @Test
    public void testRandomRead() throws Exception {
        final int READ_BUFFER_SIZE = 4 * 1024;
        final long fileSize = FileUtil.getFileSizeExceedingMemory(getContext(), READ_BUFFER_SIZE);
        if (fileSize == 0) { // not enough space, give up
            return;
        }
        String streamName = "test_random_read";
        DeviceReportLog report = new DeviceReportLog(REPORT_LOG_NAME, streamName);
        double mbps = FileUtil.doRandomReadTest(getContext(), DIR_RANDOM_RD, report, fileSize,
                READ_BUFFER_SIZE);
        report.submit(getInstrumentation());
        if (MediaPerformanceClassUtils.isPerfClass()) {
            assertTrue("measured " + mbps + " is less than target (" + MIN_READ_MBPS + " MBPS)",
                       mbps >= MIN_READ_MBPS);
        }
    }

    // It is taking too long in some device, and thus cannot run multiple times
    @CddTest(requirement="8.2")
    @Test
    public void testRandomUpdate() throws Exception {
        final int WRITE_BUFFER_SIZE = 4 * 1024;
        final long usableSpace = Environment.getDataDirectory().getUsableSpace();
        long fileSize = 256 * 1024 * 1024;
        while (usableSpace < fileSize) {
            fileSize = fileSize / 2;
        }
        String streamName = "test_random_update";
        DeviceReportLog report = new DeviceReportLog(REPORT_LOG_NAME, streamName);
        double mbps = -1;
        // this is in-fact true
        if (fileSize > FileUtil.BUFFER_SIZE) {
            mbps = FileUtil.doRandomWriteTest(getContext(), DIR_RANDOM_WR, report, fileSize,
                WRITE_BUFFER_SIZE);
        }
        report.submit(getInstrumentation());
        if (MediaPerformanceClassUtils.isPerfClass()) {
            // for performance class devices we must be able to write 256MB
            assertTrue("measured " + mbps + " is less than target (" + MIN_WRITE_MBPS + " MBPS)",
                       mbps >= MIN_WRITE_MBPS);
        }
    }
}
