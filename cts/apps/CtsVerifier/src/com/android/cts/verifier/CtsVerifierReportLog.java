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

package com.android.cts.verifier;

import android.os.Environment;
import android.util.Log;

import com.android.compatibility.common.util.ReportLog;
import com.android.compatibility.common.util.ReportLogDeviceInfoStore;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;

import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.File;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.List;

public class CtsVerifierReportLog extends ReportLog {
    private static final String TAG = CtsVerifierReportLog.class.getSimpleName();
    private static final boolean DEBUG = false;

    private ReportLogDeviceInfoStore mStore;

    private static final String LOG_ERROR_STR = "Could not log metric.";

    public CtsVerifierReportLog(String reportLogName, String streamName) {
        this(reportLogName, streamName, new File(Environment.getExternalStorageDirectory(),
                ReportExporter.LOGS_DIRECTORY));
    }

    public CtsVerifierReportLog(String reportLogName, String streamName, File logDirectory) {
        super(reportLogName, streamName);
        if (DEBUG) {
            Log.d(TAG, "CtsVerifierReportLog()");
            Log.d(TAG, "  reportLogName:" + reportLogName);
            Log.d(TAG, "  streamName:" + streamName);
            Log.d(TAG, "  logDirectory:" + logDirectory);
        }

        try {
            // dir value must match the src-dir value configured in ReportLogCollector target
            // preparer in cts/harness/tools/cts-tradefed/res/config/cts-preconditions.xml
            if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                throw new IOException("External storage is not mounted");
            } else if ((!logDirectory.exists() && !logDirectory.mkdirs())
                    || (logDirectory.exists() && !logDirectory.isDirectory())) {
                throw new IOException("Cannot create directory for device info files");
            } else {
                File jsonFile = new File(logDirectory, mReportLogName + ".reportlog.json");
                mStore = new ReportLogDeviceInfoStore(jsonFile, mStreamName);
                mStore.open();
            }
        } catch (Exception e) {
            Log.e(TAG, "Could not create report log file.", e);
        }
    }

    /**
     * Closes report file. Static functions that do not have access to instrumentation can
     * use this to close report logs. Summary, if present, is not reported to instrumentation, hence
     * does not appear in the result XML.
     */
    public void submit() {
        if (DEBUG) {
            Log.d(TAG, "submit()");
        }
        try {
            mStore.close();
        } catch (Exception e) {
            Log.e(TAG, "ReportLog Submit Failed", e);
        }
    }

    //
    // ReportLog overrides
    //
    /**
     * Adds a double array of metrics to the report.
     */
    @Override
    public void addValues(String message, double[] values, ResultType type, ResultUnit unit) {
        // Do nothing. Subclasses may implement using InfoStore to write metrics to files.
        Log.e(TAG, "No implementation for addValues(double[])");
    }

    /**
     * Adds a double array of metrics to the report.
     */
    @Override
    public void addValues(String source, String message, double[] values, ResultType type,
                          ResultUnit unit) {
        // Do nothing. Subclasses may implement using InfoStore to write metrics to files.
        Log.e(TAG, "No implementation for addValues(double[])");
    }

    /**
     * Adds a double metric to the report.
     */
    @Override
    public void addValue(String message, double value, ResultType type, ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (double) ", e);
        }
    }

    /**
     * Adds a double metric to the report.
     */
    @Override
    public void addValue(String source, String message, double value, ResultType type,
                         ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (double) ", e);
        }
    }

    /**
     * Adds an int metric to the report.
     */
    @Override
    public void addValue(String message, int value, ResultType type, ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (int) ", e);
        }
    }

    /**
     * Adds a long metric to the report.
     */
    @Override
    public void addValue(String message, long value, ResultType type, ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (long) ", e);
        }
    }

    /**
     * Adds a float metric to the report.
     */
    @Override
    public void addValue(String message, float value, ResultType type, ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (float) ", e);
        }
    }

    /**
     * Adds a boolean metric to the report.
     */
    @Override
    public void addValue(String message, boolean value, ResultType type, ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (boolean) ", e);
        }
    }

    /**
     * Adds a String metric to the report.
     */
    @Override
    public void addValue(String message, String value, ResultType type, ResultUnit unit) {
        try {
            mStore.addResult(message, value);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (String) ", e);
        }
    }

    /**
     * Adds an int array of metrics to the report.
     */
    @Override
    public void addValues(String message, int[] values, ResultType type, ResultUnit unit) {
        try {
            mStore.addArrayResult(message, values);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (int[]) ", e);
        }
    }

    /**
     * Adds a long array of metrics to the report.
     */
    @Override
    public void addValues(String message, long[] values, ResultType type, ResultUnit unit) {
        try {
            mStore.addArrayResult(message, values);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (long[]) ", e);
        }
    }

    /**
     * Adds a float array of metrics to the report.
     */
    @Override
    public void addValues(String message, float[] values, ResultType type, ResultUnit unit) {
        try {
            mStore.addArrayResult(message, values);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (float[]) ", e);
        }
    }

    /**
     * Adds a boolean array of metrics to the report.
     */
    @Override
    public void addValues(String message, boolean[] values, ResultType type, ResultUnit unit) {
        try {
            mStore.addArrayResult(message, values);
        } catch (Exception e) {
            Log.e(TAG, LOG_ERROR_STR + " (boolean[]) ", e);
        }
    }

    /**
     * Adds a String List of metrics to the report.
     */
    @Override
    public void addValues(String message, List<String> values, ResultType type, ResultUnit unit) {
        Log.e(TAG, "No implementation for addValues(String, List<String>, ResultType, ResultUnit)");
    }

    public static String serialize(ReportLog reportlog) throws XmlPullParserException,
            IllegalArgumentException, IllegalStateException, IOException {
        Log.e(TAG, "No implementation for serialize(ReportLog)");
        return "";
    }

    public static void serialize(XmlSerializer serializer, ReportLog reportLog)
            throws IOException {
        Log.e(TAG, "No implementation for serialize(XmlSerializer, ReportLog)");
    }

    //
    // It is not understood why these methods are called (through reflection), but an
    // exception is thrown if they are not implemented.
    //
    // Serializable readObject
    private void readObject(ObjectInputStream inputStream) throws ClassNotFoundException,
            IOException {
    }

    // Serializable writeObject
    private void writeObject(ObjectOutputStream outputStream) throws IOException {
    }
}
