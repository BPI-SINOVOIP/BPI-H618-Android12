/*
 * Copyright (C) 2010 The Android Open Source Project
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

import static com.android.cts.verifier.TestListActivity.sCurrentDisplayMode;
import static com.android.cts.verifier.TestListActivity.sInitialLaunch;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.android.compatibility.common.util.ReportLog;
import com.android.cts.verifier.TestListActivity.DisplayMode;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * {@link BaseAdapter} that handles loading, refreshing, and setting test
 * results. What tests are shown can be customized by overriding
 * {@link #getRows()}. See {@link ArrayTestListAdapter} and
 * {@link ManifestTestListAdapter} for examples.
 */
public abstract class TestListAdapter extends BaseAdapter {

    /** Activities implementing {@link Intent#ACTION_MAIN} and this will appear in the list. */
    public static final String CATEGORY_MANUAL_TEST = "android.cts.intent.category.MANUAL_TEST";

    /** View type for a category of tests like "Sensors" or "Features" */
    private static final int CATEGORY_HEADER_VIEW_TYPE = 0;

    /** View type for an actual test like the Accelerometer test. */
    private static final int TEST_VIEW_TYPE = 1;

    /** Padding around the text views and icons. */
    private static final int PADDING = 10;

    private final Context mContext;

    /** Immutable data of tests like the test's title and launch intent. */
    private final List<TestListItem> mRows = new ArrayList<TestListItem>();

    /** Mutable test results that will change as each test activity finishes. */
    private final Map<String, Integer> mTestResults = new HashMap<String, Integer>();

    /** Map from test name to test details. */
    private final Map<String, String> mTestDetails = new HashMap<String, String>();

    /** Map from test name to {@link ReportLog}. */
    private final Map<String, ReportLog> mReportLogs = new HashMap<String, ReportLog>();

    /** Map from test name to {@link TestResultHistoryCollection}. */
    private final Map<String, TestResultHistoryCollection> mHistories = new HashMap<>();

    private final LayoutInflater mLayoutInflater;

    /** Map from display mode to the list of {@link TestListItem}.
     *  Records the TestListItem from main view only, including unfolded mode and folded mode
     *  respectively. */
    protected Map<String, List<TestListItem>> mDisplayModesTests = new HashMap<>();

    /** Flag to identify the test data from {@link ManifestTestListAdapter}.
     *  The source of data for the adapter is various, such as ManifestTestListAdapter and
     *  ArrayTestListAdapter, and the data of foldable tests are from ManifestTestListAdapter. */
    protected static boolean adapterFromManifest;

    /** Flag to identify the test data in main view from {@link ManifestTestListAdapter}.
     *  ManifestTestListAdapter provides test data for main view and subtests.
     *  Getting foldable tests is from main view only. */
    protected static boolean hasTestParentInManifestAdapter;

    /** {@link ListView} row that is either a test category header or a test. */
    public static class TestListItem {

        /** Title shown in the {@link ListView}. */
        final String title;

        /** Test name with class and test ID to uniquely identify the test. Null for categories. */
        String testName;

        /** Intent used to launch the activity from the list. Null for categories. */
        final Intent intent;

        /** Features necessary to run this test. */
        final String[] requiredFeatures;

        /** Configs necessary to run this test. */
        final String[] requiredConfigs;

        /** Intent actions necessary to run this test. */
        final String[] requiredActions;

        /** Features such that, if any present, the test gets excluded from being shown. */
        final String[] excludedFeatures;

        /** If any of of the features are present the test is meaningful to run. */
        final String[] applicableFeatures;

        /** Configs display mode to run this test. */
        final String displayMode;

        // TODO: refactor to use a Builder approach instead

        public static TestListItem newTest(Context context, int titleResId, String testName,
            Intent intent, String[] requiredFeatures, String[] excludedFeatures,
            String[] applicableFeatures) {
            return newTest(context.getString(titleResId), testName, intent, requiredFeatures,
                excludedFeatures, applicableFeatures);
        }

        public static TestListItem newTest(Context context, int titleResId, String testName,
                Intent intent, String[] requiredFeatures, String[] excludedFeatures) {
            return newTest(context.getString(titleResId), testName, intent, requiredFeatures,
                    excludedFeatures, /* applicableFeatures= */ null);
        }

        public static TestListItem newTest(Context context, int titleResId, String testName,
                Intent intent, String[] requiredFeatures) {
            return newTest(context.getString(titleResId), testName, intent, requiredFeatures,
                    /* excludedFeatures= */ null, /* applicableFeatures= */ null);
        }

        public static TestListItem newTest(String title, String testName, Intent intent,
                String[] requiredFeatures, String[] requiredConfigs, String[] requiredActions,
                String[] excludedFeatures, String[] applicableFeatures, String displayMode) {
            return new TestListItem(title, testName, intent, requiredFeatures, requiredConfigs,
                    requiredActions, excludedFeatures, applicableFeatures, displayMode);
        }

        public static TestListItem newTest(String title, String testName, Intent intent,
            String[] requiredFeatures, String[] requiredConfigs, String[] excludedFeatures,
            String[] applicableFeatures) {
            return new TestListItem(title, testName, intent, requiredFeatures, requiredConfigs,
                    /* requiredActions = */ null, excludedFeatures, applicableFeatures,
                    /* displayMode= */ null);
        }

        public static TestListItem newTest(String title, String testName, Intent intent,
                String[] requiredFeatures, String[] excludedFeatures, String[] applicableFeatures) {
            return new TestListItem(title, testName, intent, requiredFeatures,
                    /* requiredConfigs= */ null, /* requiredActions = */ null, excludedFeatures,
                    applicableFeatures, /* displayMode= */ null);
        }

        public static TestListItem newTest(String title, String testName, Intent intent,
                String[] requiredFeatures, String[] excludedFeatures) {
            return new TestListItem(title, testName, intent, requiredFeatures,
                    /* requiredConfigs= */ null, /* requiredActions = */ null, excludedFeatures,
                    /* applicableFeatures= */ null, /* displayMode= */ null);
        }

        public static TestListItem newTest(String title, String testName, Intent intent,
                String[] requiredFeatures) {
            return new TestListItem(title, testName, intent, requiredFeatures,
                    /* requiredConfigs= */ null, /* requiredActions = */ null,
                    /* excludedFeatures= */ null, /* applicableFeatures= */ null,
                    /* displayMode= */ null);
        }

        public static TestListItem newCategory(Context context, int titleResId) {
            return newCategory(context.getString(titleResId));
        }

        public static TestListItem newCategory(String title) {
            return new TestListItem(title, /* testName= */ null, /* intent= */ null,
                    /* requiredFeatures= */ null,  /* requiredConfigs= */ null,
                    /* requiredActions = */ null, /* excludedFeatures= */ null,
                    /* applicableFeatures= */ null, /* displayMode= */ null);
        }

        protected TestListItem(String title, String testName, Intent intent,
                String[] requiredFeatures, String[] excludedFeatures, String[] applicableFeatures) {
            this(title, testName, intent, requiredFeatures, /* requiredConfigs= */ null,
                    /* requiredActions = */ null, excludedFeatures, applicableFeatures,
                    /* displayMode= */ null);
        }

        protected TestListItem(String title, String testName, Intent intent,
                String[] requiredFeatures, String[] requiredConfigs, String[] requiredActions,
                String[] excludedFeatures, String[] applicableFeatures, String displayMode) {
            this.title = title;
            if (!sInitialLaunch) {
                testName = setTestNameSuffix(sCurrentDisplayMode, testName);
            }
            this.testName = testName;
            this.intent = intent;
            this.requiredActions = requiredActions;
            this.requiredFeatures = requiredFeatures;
            this.requiredConfigs = requiredConfigs;
            this.excludedFeatures = excludedFeatures;
            this.applicableFeatures = applicableFeatures;
            this.displayMode = displayMode;
        }

        boolean isTest() {
            return intent != null;
        }
    }

    public TestListAdapter(Context context) {
        this.mContext = context;
        this.mLayoutInflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        TestResultContentObserver observer = new TestResultContentObserver();
        ContentResolver resolver = context.getContentResolver();
        resolver.registerContentObserver(TestResultsProvider.getResultContentUri(context), true, observer);
    }

    public void loadTestResults() {
        new RefreshTestResultsTask().execute();
    }

    public void clearTestResults() {
        new ClearTestResultsTask().execute();
    }

    public void setTestResult(TestResult testResult) {
        String name = testResult.getName();

        // Append existing history
        TestResultHistoryCollection histories = testResult.getHistoryCollection();
        histories.merge(null, mHistories.get(name));

        new SetTestResultTask(name, testResult.getResult(),
                testResult.getDetails(), testResult.getReportLog(), histories).execute();
    }

    class RefreshTestResultsTask extends AsyncTask<Void, Void, RefreshResult> {
        @Override
        protected RefreshResult doInBackground(Void... params) {
            List<TestListItem> rows;
            // When initial launch, needs to fetch tests in the unfolded/folded mode
            // to be stored in mDisplayModesTests as the basis for the future switch.
            if (sInitialLaunch) {
                getRows();
                sInitialLaunch = false;
            }

            if (checkTestsFromMainView()) {
                rows = mDisplayModesTests.get(sCurrentDisplayMode);
            }else {
                rows = getRows();
            }
            return getRefreshResults(rows);
        }

        @Override
        protected void onPostExecute(RefreshResult result) {
            super.onPostExecute(result);
            mRows.clear();
            mRows.addAll(result.mItems);
            mTestResults.clear();
            mTestResults.putAll(result.mResults);
            mTestDetails.clear();
            mTestDetails.putAll(result.mDetails);
            mReportLogs.clear();
            mReportLogs.putAll(result.mReportLogs);
            mHistories.clear();
            mHistories.putAll(result.mHistories);
            notifyDataSetChanged();
        }
    }

    static class RefreshResult {
        List<TestListItem> mItems;
        Map<String, Integer> mResults;
        Map<String, String> mDetails;
        Map<String, ReportLog> mReportLogs;
        Map<String, TestResultHistoryCollection> mHistories;

        RefreshResult(
                List<TestListItem> items,
                Map<String, Integer> results,
                Map<String, String> details,
                Map<String, ReportLog> reportLogs,
                Map<String, TestResultHistoryCollection> histories) {
            mItems = items;
            mResults = results;
            mDetails = details;
            mReportLogs = reportLogs;
            mHistories = histories;
        }
    }

    protected abstract List<TestListItem> getRows();

    static final String[] REFRESH_PROJECTION = {
        TestResultsProvider._ID,
        TestResultsProvider.COLUMN_TEST_NAME,
        TestResultsProvider.COLUMN_TEST_RESULT,
        TestResultsProvider.COLUMN_TEST_DETAILS,
        TestResultsProvider.COLUMN_TEST_METRICS,
        TestResultsProvider.COLUMN_TEST_RESULT_HISTORY,
    };

    RefreshResult getRefreshResults(List<TestListItem> items) {
        Map<String, Integer> results = new HashMap<String, Integer>();
        Map<String, String> details = new HashMap<String, String>();
        Map<String, ReportLog> reportLogs = new HashMap<String, ReportLog>();
        Map<String, TestResultHistoryCollection> histories = new HashMap<>();
        ContentResolver resolver = mContext.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = resolver.query(TestResultsProvider.getResultContentUri(mContext), REFRESH_PROJECTION,
                    null, null, null);
            if (cursor.moveToFirst()) {
                do {
                    String testName = cursor.getString(1);
                    int testResult = cursor.getInt(2);
                    String testDetails = cursor.getString(3);
                    ReportLog reportLog = (ReportLog) deserialize(cursor.getBlob(4));
                    TestResultHistoryCollection historyCollection =
                        (TestResultHistoryCollection) deserialize(cursor.getBlob(5));
                    results.put(testName, testResult);
                    details.put(testName, testDetails);
                    reportLogs.put(testName, reportLog);
                    histories.put(testName, historyCollection);
                } while (cursor.moveToNext());
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return new RefreshResult(items, results, details, reportLogs, histories);
    }

    class ClearTestResultsTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            ContentResolver resolver = mContext.getContentResolver();
            resolver.delete(TestResultsProvider.getResultContentUri(mContext), "1", null);
            return null;
        }
    }

    class SetTestResultTask extends AsyncTask<Void, Void, Void> {

        private final String mTestName;
        private final int mResult;
        private final String mDetails;
        private final ReportLog mReportLog;
        private final TestResultHistoryCollection mHistoryCollection;

        SetTestResultTask(
                String testName,
                int result,
                String details,
                ReportLog reportLog,
                TestResultHistoryCollection historyCollection) {
            mTestName = testName;
            mResult = result;
            mDetails = details;
            mReportLog = reportLog;
            mHistoryCollection = historyCollection;
        }

        @Override
        protected Void doInBackground(Void... params) {
            TestResultsProvider.setTestResult(
                mContext, mTestName, mResult, mDetails, mReportLog, mHistoryCollection);
            return null;
        }
    }

    class TestResultContentObserver extends ContentObserver {

        public TestResultContentObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            loadTestResults();
        }
    }

    @Override
    public boolean areAllItemsEnabled() {
        // Section headers for test categories are not clickable.
        return false;
    }

    @Override
    public boolean isEnabled(int position) {
        return getItem(position).isTest();
    }

    @Override
    public int getItemViewType(int position) {
        return getItem(position).isTest() ? TEST_VIEW_TYPE : CATEGORY_HEADER_VIEW_TYPE;
    }

    @Override
    public int getViewTypeCount() {
        return 2;
    }

    @Override
    public int getCount() {
        if (!sInitialLaunch && checkTestsFromMainView()) {
            return mDisplayModesTests.get(sCurrentDisplayMode).size();
        }
        return mRows.size();
    }

    @Override
    public TestListItem getItem(int position) {
        if (checkTestsFromMainView()) {
            return mDisplayModesTests.get(sCurrentDisplayMode).get(position);
        }
        return mRows.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public int getTestResult(int position) {
        TestListItem item = getItem(position);
        return mTestResults.containsKey(item.testName)
                ? mTestResults.get(item.testName)
                : TestResult.TEST_RESULT_NOT_EXECUTED;
    }

    public String getTestDetails(int position) {
        TestListItem item = getItem(position);
        return mTestDetails.containsKey(item.testName)
                ? mTestDetails.get(item.testName)
                : null;
    }

    public ReportLog getReportLog(int position) {
        TestListItem item = getItem(position);
        return mReportLogs.containsKey(item.testName)
                ? mReportLogs.get(item.testName)
                : null;
    }

    /**
     * Get test result histories.
     *
     * @param position The position of test.
     * @return A {@link TestResultHistoryCollection} object containing test result histories of tests.
     */
    public TestResultHistoryCollection getHistoryCollection(int position) {
        TestListItem item = getItem(position);
        return mHistories.containsKey(item.testName)
            ? mHistories.get(item.testName)
            : null;
    }

    /**
     * Get test item by the given display mode and position.
     *
     * @param mode The display mode.
     * @param position The position of test.
     * @return A {@link TestListItem} object containing the test item.
     */
    public TestListItem getItem(String mode, int position) {
        return mDisplayModesTests.get(mode).get(position);
    }

    /**
     * Get test item count by the given display mode.
     *
     * @param mode The display mode.
     * @return A count of test items.
     */
    public int getCount(String mode){
        return mDisplayModesTests.get(mode).size();
    }

    /**
     * Get test result by the given display mode and position.
     *
     * @param mode The display mode.
     * @param position The position of test.
     * @return The test item result.
     */
    public int getTestResult(String mode, int position) {
        TestListItem item = mDisplayModesTests.get(mode).get(position);
        return mTestResults.containsKey(item.testName)
            ? mTestResults.get(item.testName)
            : TestResult.TEST_RESULT_NOT_EXECUTED;
    }

    /**
     * Get test details by the given display mode and position.
     *
     * @param mode The display mode.
     * @param position The position of test.
     * @return A string containing the test details.
     */
    public String getTestDetails(String mode, int position) {
        TestListItem item = mDisplayModesTests.get(mode).get(position);
        return mTestDetails.containsKey(item.testName)
            ? mTestDetails.get(item.testName)
            : null;
    }

    /**
     * Get test report log by the given display mode and position.
     *
     * @param mode The display mode.
     * @param position The position of test.
     * @return A {@link ReportLog} object containing the test report log of the test item.
     */
    public ReportLog getReportLog(String mode, int position) {
        TestListItem item = mDisplayModesTests.get(mode).get(position);
        return mReportLogs.containsKey(item.testName)
            ? mReportLogs.get(item.testName)
            : null;
    }

    /**
     * Get test result histories by the given display mode and position.
     *
     * @param mode The display mode.
     * @param position The position of test.
     * @return A {@link TestResultHistoryCollection} object containing the test result histories of
     *         the test item.
     */
    public TestResultHistoryCollection getHistoryCollection(String mode, int position) {
        TestListItem item = mDisplayModesTests.get(mode).get(position);
        return mHistories.containsKey(item.testName)
            ? mHistories.get(item.testName)
            : null;
    }

    public boolean allTestsPassed() {
        for (TestListItem item : mRows) {
            if (item.isTest() && (!mTestResults.containsKey(item.testName)
                    || (mTestResults.get(item.testName) != TestResult.TEST_RESULT_PASSED))) {
                return false;
            }
        }
        return true;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        TextView textView;
        if (convertView == null) {
            int layout = getLayout(position);
            textView = (TextView) mLayoutInflater.inflate(layout, parent, false);
        } else {
            textView = (TextView) convertView;
        }

        TestListItem item = getItem(position);
        textView.setText(item.title);
        textView.setPadding(PADDING, 0, PADDING, 0);
        textView.setCompoundDrawablePadding(PADDING);

        if (item.isTest()) {
            int testResult = getTestResult(position);
            int backgroundResource = 0;
            int iconResource = 0;

            /** TODO: Remove fs_ prefix from feature icons since they are used here too. */
            switch (testResult) {
                case TestResult.TEST_RESULT_PASSED:
                    backgroundResource = R.drawable.test_pass_gradient;
                    iconResource = R.drawable.fs_good;
                    break;

                case TestResult.TEST_RESULT_FAILED:
                    backgroundResource = R.drawable.test_fail_gradient;
                    iconResource = R.drawable.fs_error;
                    break;

                case TestResult.TEST_RESULT_NOT_EXECUTED:
                    break;

                default:
                    throw new IllegalArgumentException("Unknown test result: " + testResult);
            }

            textView.setBackgroundResource(backgroundResource);
            textView.setCompoundDrawablesWithIntrinsicBounds(0, 0, iconResource, 0);
        }

        return textView;
    }

    private int getLayout(int position) {
        int viewType = getItemViewType(position);
        switch (viewType) {
            case CATEGORY_HEADER_VIEW_TYPE:
                return R.layout.test_category_row;
            case TEST_VIEW_TYPE:
                return android.R.layout.simple_list_item_1;
            default:
                throw new IllegalArgumentException("Illegal view type: " + viewType);

        }
    }

    public static Object deserialize(byte[] bytes) {
        if (bytes == null || bytes.length == 0) {
            return null;
        }
        ByteArrayInputStream byteStream = new ByteArrayInputStream(bytes);
        ObjectInputStream objectInput = null;
        try {
            objectInput = new ObjectInputStream(byteStream);
            return objectInput.readObject();
        } catch (IOException e) {
            return null;
        } catch (ClassNotFoundException e) {
            return null;
        } finally {
            try {
                if (objectInput != null) {
                    objectInput.close();
                }
                byteStream.close();
            } catch (IOException e) {
                // Ignore close exception.
            }
        }
    }

    /**
     * Sets test name suffix. In the folded mode, the suffix is [folded]; otherwise, it is empty
     * string.
     *
     * @param mode A string of current display mode.
     * @param name A string of test name.
     * @return A string of test name with suffix, [folded], in the folded mode.
     *         A string of input test name in the unfolded mode.
     */
    public static String setTestNameSuffix(String mode, String name) {
        if (name != null && mode.equals(DisplayMode.FOLDED.toString())
            && !name.endsWith(DisplayMode.FOLDED.asSuffix())){
            return name + DisplayMode.FOLDED.asSuffix();
        }
        return name;
    }

    /**
     * Checks if the tests are from main view for foldable tests.
     *
     * @return True if the tests from main view, otherwise, return false.
     */
    private static boolean checkTestsFromMainView() {
        return adapterFromManifest && !hasTestParentInManifestAdapter;
    }
}
