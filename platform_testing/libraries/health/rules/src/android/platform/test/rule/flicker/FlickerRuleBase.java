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

package android.platform.test.rule.flicker;

import android.os.Environment;
import android.platform.test.rule.TestWatcher;
import android.util.Log;

import com.android.server.wm.flicker.monitor.LayersTraceMonitor;
import com.android.server.wm.flicker.monitor.WindowManagerTraceMonitor;
import com.android.server.wm.traces.common.layers.LayersTrace;
import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace;
import com.android.server.wm.traces.parser.layers.LayersTraceParser;
import com.android.server.wm.traces.parser.windowmanager.WindowManagerTraceParser;

import com.google.common.io.Files;
import java.util.HashMap;
import java.util.Map;
import java.nio.file.Paths;
import java.io.File;
import org.junit.runner.Description;

/**
 * Base class that encapsulates the logic for collecting and parsing
 * the window manager trace and surface flinger trace.
 */
public abstract class FlickerRuleBase extends TestWatcher {

    private static final String TAG = FlickerRuleBase.class.getSimpleName();

    // Suffix is added by the trace monitor at the time of saving the file.
    private static final String WM_TRACE_FILE_SUFFIX = "wm_trace.pb";

    // Suffix is added by the trace monitor at the time of saving the file.
    private static final String LAYERS_TRACE_FILE_SUFFIX = "layers_trace.pb";

    // Option to customize the root directory to save the trace files.
    private static final String TRACE_DIRECTORY_ROOT = "trace-directory-root";

    // To keep track of the method name and the current iteration count.
    private static Map<String, Integer> mMethodNameCount = new HashMap<>();
    private static boolean updateMethodCount = true;
    private boolean mIsWmTraceEnabled = false;
    private boolean mIsLayersTraceEnabled = false;
    private WindowManagerTraceMonitor mWmTraceMonitor;
    private LayersTraceMonitor mLayersTraceMonitor;
    private String mTraceDirectoryRoot = null;
    private boolean mIsRuleDisabled= false;
    private WindowManagerTrace mWindowManagerTrace;
    private LayersTrace mLayerTrace;

    public FlickerRuleBase() {
    }

    @Override
    protected void starting(Description description) {
        if (!mIsWmTraceEnabled && !mIsLayersTraceEnabled) {
            Log.v(TAG, "Both surface flinger and layer tracing are disabled.");
            return;
        }

        setupRootDirectory();

        // Verify if WM tracing is already started by another window manager based
        // rule otherwise proceed with starting the WM trace.
        if (mIsWmTraceEnabled) {
            if (!mWmTraceMonitor.isEnabled()) {
                mWmTraceMonitor.start();
                Log.v(TAG, "WM trace started successfully.");
            } else {
                Log.v(TAG, "WM trace already enabled.");
            }
        }

        // Verify if layers tracing is already started by another layers based
        // rule otherwise proceed with starting the layer trace.
        if (mIsLayersTraceEnabled) {
            if (!mLayersTraceMonitor.isEnabled()) {
                mLayersTraceMonitor.start();
                Log.v(TAG, "Layers trace started successfully.");
            } else {
                Log.v(TAG, "Layers trace already enabled.");
            }
        }

        // If traces are already started by other window manager or layer based rule
        // then exit without incrementing the method iteration count.
        if (!updateMethodCount) {
            return;
        }

        // Update the method name with current iteration count.
        if (mMethodNameCount.containsKey(description.toString())) {
            mMethodNameCount.put(description.toString(),
                    mMethodNameCount.get(description.toString()) + 1);
        } else {
            mMethodNameCount.put(description.toString(), 1);
        }

        // Cleanup the trace file from previous test runs.
        cleanupTraceFiles(description);

        updateMethodCount = false;
    }

    @Override
    protected void finished(Description description) {

        updateMethodCount = true;

        // Verify if WM tracing is already stopped by another window manager based
        // rule. Otherwise proceed with stopping the trace.
        if (mIsWmTraceEnabled) {
            if (mWmTraceMonitor.isEnabled()) {
                mWmTraceMonitor.stop();
                Log.v(TAG, "WM trace stopped successfully.");
            } else {
                Log.v(TAG, "WM trace already stopped.");
            }
        }

        if (mIsWmTraceEnabled) {
            // Verify if WM trace file already exist for the current test. It could have been
            // created by another Window manager based rule.
            if (!new File(getFinalTraceFilePath(description, WM_TRACE_FILE_SUFFIX)).exists()) {
                // Appends the trace file suffix "_wm_trace.pb" and store it under the root
                // directory.
                mWmTraceMonitor.save(getFileNamePrefix(description));
                Log.v(TAG, "WM trace successfully saved in the destination folder.");
            } else {
                Log.v(TAG, "WM trace already saved in the destination folder.");
            }
            mWindowManagerTrace = getWindowManagerTrace(
                    getFinalTraceFilePath(description, WM_TRACE_FILE_SUFFIX));
        }

        // Verify if layers tracing is already stopped by another layers based
        // rule. Otherwise proceed with stopping the trace.
        if (mIsLayersTraceEnabled) {
            if (mLayersTraceMonitor.isEnabled()) {
                mLayersTraceMonitor.stop();
                Log.v(TAG, "Layers trace stopped successfully.");
            } else {
                Log.v(TAG, "Layers trace already stopped.");
            }
        }

        if (mIsLayersTraceEnabled) {
            // Verify if layer trace file already exist for the current test. It could have been
            // created by another layers based rule.
            if (!new File(getFinalTraceFilePath(description, LAYERS_TRACE_FILE_SUFFIX)).exists()) {
                // Appends the trace file suffix "_layers_trace.pb" and store it under the root
                // directory.
                mLayersTraceMonitor.save(getFileNamePrefix(description));
                Log.v(TAG, "Layers trace successfully saved in the destination folder.");
            } else {
                Log.v(TAG, "Layers trace already saved in the destination folder.");
            }
            mLayerTrace = getLayersTrace(
                    getFinalTraceFilePath(description, LAYERS_TRACE_FILE_SUFFIX));
        }

        validateFlickerConditions();
    }

    /**
    * Setup the root directory to save the WM traces and layers trace collected during the test.
    */
    private void setupRootDirectory() {
        mTraceDirectoryRoot = getArguments().getString(TRACE_DIRECTORY_ROOT,
                Environment.getExternalStorageDirectory().getPath() + "/flicker_trace/");
        if (!mTraceDirectoryRoot.endsWith("/")) {
            mTraceDirectoryRoot = new StringBuilder(mTraceDirectoryRoot).append("/").toString();
        }

        // Create root directory if it does not already exist.
        File rootDirectory = new File(mTraceDirectoryRoot);
        if (!rootDirectory.exists()) {
            if (rootDirectory.mkdirs()) {
                Log.v(TAG, "Trace root directory created successfully.");
            } else {
                throw new RuntimeException(
                        "Unable to create the trace root directory." + mTraceDirectoryRoot);
            }
        } else {
            Log.v(TAG, "Trace root directory already exists.");
        }

        if (mIsWmTraceEnabled) {
            mWmTraceMonitor = new WindowManagerTraceMonitor(Paths.get(mTraceDirectoryRoot));
        }

        if (mIsLayersTraceEnabled) {
            mLayersTraceMonitor = new LayersTraceMonitor(Paths.get(mTraceDirectoryRoot));
        }
    }

    /**
     * Remove the WM trace and layers trace files collected from previous test runs.
     */
    private void cleanupTraceFiles(Description description) {
        if (new File(getFinalTraceFilePath(description, WM_TRACE_FILE_SUFFIX)).exists()) {
            new File(getFinalTraceFilePath(description, WM_TRACE_FILE_SUFFIX)).delete();
            Log.v(TAG, "Removed the already existing wm trace file.");
        }

        if (new File(getFinalTraceFilePath(description, LAYERS_TRACE_FILE_SUFFIX)).exists()) {
            new File(getFinalTraceFilePath(description, LAYERS_TRACE_FILE_SUFFIX)).delete();
            Log.v(TAG, "Removed the already existing layers trace file.");
        }
    }

    /**
     * Retrieve the path of the trace file in the device.
     *
     * @param description
     * @param suffix
     * @return path to the trace file.
     */
    private String getFinalTraceFilePath(Description description, String suffix) {
        return String.format(
                "%s%s_%s", mTraceDirectoryRoot, getFileNamePrefix(description),
                suffix);
    }

    /**
     * Construct file name using the class name, method name and the current iteration count
     * of the method.
     *
     * @param description
     * @return the file name used to save the WM trace proto file.
     */
    private String getFileNamePrefix(Description description) {
        return description.getClassName() + "_" + description.getMethodName() + "_"
                + mMethodNameCount.get(description.toString());
    }

    /**
     * Parse the window manager trace file.
     *
     * @param finalTraceFilePath
     * @return parsed window manager trace.
     */
    private WindowManagerTrace getWindowManagerTrace(String finalTraceFilePath) {
        Log.v(TAG, "Processing window manager trace file.");
        try {
            byte[] wmTraceByteArray = Files.toByteArray(new File(finalTraceFilePath));
            if (wmTraceByteArray != null) {
                WindowManagerTrace wmTrace = WindowManagerTraceParser
                        .parseFromTrace(wmTraceByteArray);
                return wmTrace;
            } else {
                throw new RuntimeException("Window manager trace contents are empty.");
            }
        } catch (Exception e) {
            throw new RuntimeException("Unable to read the proto file." + finalTraceFilePath);
        }
    }

    /**
     * Parse the layers trace file.
     *
     * @param finalTraceFilePath
     * @return parsed layers trace.
     */
    private LayersTrace getLayersTrace(String finalTraceFilePath) {
        Log.v(TAG, "Processing layers trace file.");
        try {
            byte[] layersTraceByteArray = Files.toByteArray(new File(finalTraceFilePath));
            if (layersTraceByteArray != null) {
                LayersTrace layersTrace = LayersTraceParser
                        .parseFromTrace(layersTraceByteArray);
                return layersTrace;
            } else {
                throw new RuntimeException("layers trace contents are empty.");
            }
        } catch (Exception e) {
            throw new RuntimeException("Unable to read the proto file." + finalTraceFilePath);
        }
    }

    /**
     * Child class should implement this method to test for flicker conditions.
     *
     */
    protected abstract void validateFlickerConditions();

    protected void enableWmTrace() {
        mIsWmTraceEnabled = true;
    }

    protected void enableLayerTrace() {
        mIsLayersTraceEnabled = true;
    }

    protected WindowManagerTrace getWindowManagerTrace() {
        return mWindowManagerTrace;
    }

    protected LayersTrace getLayersTrace() {
        return mLayerTrace;
    }
}
