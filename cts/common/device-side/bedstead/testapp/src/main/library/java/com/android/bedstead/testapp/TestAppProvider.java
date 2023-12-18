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

package com.android.bedstead.testapp;

import android.content.Context;

import com.android.bedstead.nene.TestApis;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashSet;
import java.util.Set;

/** Entry point to Test App. Used for querying for {@link TestApp} instances. */
public final class TestAppProvider {

    private static final TestApis sTestApis = new TestApis();
    // Must be instrumentation context to access resources
    private static final Context sContext = sTestApis.context().instrumentationContext();

    private boolean mTestAppsInitialised = false;
    private final Set<TestAppDetails> mTestApps = new HashSet<>();

    /** Begin a query for a {@link TestApp}. */
    public TestAppQueryBuilder query() {
        return new TestAppQueryBuilder(this);
    }

    /** Get any {@link TestApp}. */
    public TestApp any() {
        return query().get();
    }

    Set<TestAppDetails> testApps() {
        initTestApps();
        return mTestApps;
    }

    private void initTestApps() {
        if (mTestAppsInitialised) {
            return;
        }
        mTestAppsInitialised = true;

        int indexId = sContext.getResources().getIdentifier(
                "raw/index", /* defType= */ null, sContext.getPackageName());

        try (InputStream inputStream = sContext.getResources().openRawResource(indexId);
             BufferedReader bufferedReader =
                     new BufferedReader(new InputStreamReader(inputStream))) {
            String apkName;
            while ((apkName = bufferedReader.readLine()) != null) {
                loadApk(apkName);
            }
        } catch (IOException e) {
            throw new RuntimeException("TODO");
        }
    }

    private void loadApk(String apkName) {
        TestAppDetails details = new TestAppDetails();
        details.mPackageName = "android." + apkName; // TODO: Actually index the package name
        details.mResourceIdentifier = sContext.getResources().getIdentifier(
                "raw/" + apkName, /* defType= */ null, sContext.getPackageName());

        mTestApps.add(details);
    }

    void markTestAppUsed(TestAppDetails testApp) {
        mTestApps.remove(testApp);
    }
}
