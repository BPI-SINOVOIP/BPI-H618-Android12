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

package com.android.helpers;

import android.util.Log;

import androidx.annotation.VisibleForTesting;
import androidx.test.InstrumentationRegistry;
import androidx.test.uiautomator.UiDevice;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Helper to collect pinned files information from the device using dumpsys.
 */
public class PinnerHelper implements ICollectorHelper<String> {
    private static final String TAG = PinnerHelper.class.getSimpleName();

    @VisibleForTesting public static final String PINNER_CMD = "dumpsys pinner";
    private static final String PINNED_FILES_TOTAL_SIZE = "Total size";
    public static final String SYSTEM_HEADER_NAME = "pinner_system";
    public static final String TOTAL_SIZE_BYTES_KEY = "pinner_total_size_bytes";
    public static final String TOTAL_FILE_COUNT_KEY = "pinner_total_files_count";
    public static final String OUTPUT_FILE_PATH_KEY = "pinner_output_file";
    public static final String PINNER_FILES_COUNT_SUFFIX = "files_count";
    private String mTestOutputDir = null;
    private String mTestOutputFile = null;

    // Map to maintain pinned files memory usage.
    private Map<String, String> mPinnerMap = new HashMap<>();

    public void setUp(String testOutputDir) {
        mTestOutputDir = testOutputDir;
    }

    @Override
    public boolean startCollecting() {
        if (mTestOutputDir == null) {
            Log.e(TAG, String.format("Invalid test setup"));
            return false;
        }

        File directory = new File(mTestOutputDir);
        String filePath = String.format("%s/pinner_snapshot%d.txt", mTestOutputDir,
                UUID.randomUUID().hashCode());
        File file = new File(filePath);

        // Make sure directory exists and file does not
        if (directory.exists()) {
            if (file.exists() && !file.delete()) {
                Log.e(TAG, String.format("Result file %s already exists and cannot be deleted",
                        filePath));
                return false;
            }
        } else {
            if (!directory.mkdirs()) {
                Log.e(TAG, String.format("Failed to create result output directory %s",
                        mTestOutputDir));
                return false;
            }
        }

        // Create an empty file to fail early in case there are no write permissions
        try {
            if (!file.createNewFile()) {
                // This should not happen unless someone created the file right after we deleted it
                Log.e(TAG,
                        String.format("Race with another user of result output file %s", filePath));
                return false;
            }
        } catch (IOException e) {
            Log.e(TAG, String.format("Failed to create result output file %s", filePath), e);
            return false;
        }

        mTestOutputFile = filePath;
        return true;
    }

    @Override
    public Map<String, String> getMetrics() {
        mPinnerMap = new HashMap<String, String>();
        // Run the pinner command.
        String pinnerOutput;
        try {
            pinnerOutput = executeShellCommand(PINNER_CMD);
            Log.i(TAG, "Pinner output:\n" + pinnerOutput);
        } catch (IOException e) {
            throw new RuntimeException("Unable to execute dumpsys pinner command", e);
        }

        // Write the pinner output to a file and update the output metrics with the
        // path to the file.
        if (mTestOutputFile != null) {
            try {
                FileWriter writer = new FileWriter(new File(mTestOutputFile), true);
                storeToFile(mTestOutputFile, pinnerOutput, writer);
                writer.close();
                mPinnerMap.put(OUTPUT_FILE_PATH_KEY, mTestOutputFile);
            } catch (IOException e) {
                Log.e(TAG, String.format("Failed to write output file %s", mTestOutputFile), e);
            }
        }

        // Parse the per file memory usage and files count from the pinner details.
        parseAndUpdatePinnerInfo(pinnerOutput);

        return mPinnerMap;
    }

    @Override
    public boolean stopCollecting() {
        return true;
    }

    /**
     * Store dumpsys raw output in a text file.
     *
     * @param fileName
     * @param pinnerOutput
     * @param writer
     * @throws RuntimeException
     */
    private void storeToFile(String fileName, String pinnerOutput, FileWriter writer)
            throws RuntimeException {
        try {
            writer.write("Pinned files details.");
            writer.write(pinnerOutput);
            writer.write('\n');
        } catch (IOException e) {
            throw new RuntimeException(String.format("Unable to write file %s ", fileName), e);
        }
    }

    /**
     * Parse the dumpsys pinner output and update the system level and per pinned app level
     * file size details and the pinner file count at the system level and app level.
     *
     * @param pinnerOutput raw output from dumpsys pinner.
     * @throws RuntimeException
     */
    private void parseAndUpdatePinnerInfo(String pinnerOutput) {
        List<String> lines = Arrays.asList(pinnerOutput.split("\\r?\\n"));
        String headerName = SYSTEM_HEADER_NAME;

        // Sample dumpsys raw output:
        //
        // /system/framework/framework.jar 35004416
        // /system/framework/arm64/boot-framework.vdex 323584
        // /system/framework/arm64/boot-framework.oat 9003008
        // Home uid=10217 active=true
          // /system_ext/priv-app/NexusLauncherRelease/NexusLauncherRelease.apk 6291456
          // /system_ext/priv-app/NexusLauncherRelease/oat/arm64/NexusLauncherRelease.vdex 126976
          // /system_ext/priv-app/NexusLauncherRelease/oat/arm64/NexusLauncherRelease.odex 69632
        // Total size: 201924608

        int currentHeaderFilesCount = 0;
        int totalFilesCount = 0;
        for (String line : lines) {
            List<String> pinnerDetails = Arrays.asList(line.trim().split("\\s+"));
            if (pinnerDetails.size() == 3) {
                // Happens only for the lines with app level header info and total size.
                // Home uid=10217 active=true
                // Total size: 201924608

                // Update the previous app pinned file count.
                mPinnerMap.put(String.format("%s_%s", headerName, PINNER_FILES_COUNT_SUFFIX),
                        String.valueOf(currentHeaderFilesCount));
                currentHeaderFilesCount = 0;
                if (line.contains(PINNED_FILES_TOTAL_SIZE)) {
                    mPinnerMap.put(TOTAL_SIZE_BYTES_KEY, pinnerDetails.get(2).trim());
                    break;
                }

                // Update the header name (i.e Home)
                headerName = "pinner_" + pinnerDetails.get(0).trim();
            } else if (pinnerDetails.size() == 2) {
                // Happens only for the lines with individual pinned file sizes.
                // /system_ext/priv-app/NexusLauncherRelease/NexusLauncherRelease.apk 6291456
                mPinnerMap.put(
                        String.format("%s_%s_bytes", headerName, pinnerDetails.get(0).trim()),
                        pinnerDetails.get(1).trim());
                currentHeaderFilesCount++;
                totalFilesCount++;
            }
        }
        // Update the previous app pinned file count.
        mPinnerMap.put(String.format(TOTAL_FILE_COUNT_KEY), String.valueOf(totalFilesCount));
    }

    /* Execute a shell command and return its output. */
    @VisibleForTesting
    public String executeShellCommand(String command) throws IOException {
        return UiDevice.getInstance(InstrumentationRegistry.getInstrumentation())
                .executeShellCommand(command);
    }
}
