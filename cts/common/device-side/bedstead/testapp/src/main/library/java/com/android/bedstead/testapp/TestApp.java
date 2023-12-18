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

import static com.android.compatibility.common.util.FileUtils.readInputStreamFully;

import android.content.Context;
import android.os.UserHandle;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.exceptions.NeneException;
import com.android.bedstead.nene.packages.Package;
import com.android.bedstead.nene.packages.PackageReference;
import com.android.bedstead.nene.users.UserReference;
import com.android.bedstead.testapp.processor.annotations.TestAppSender;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/** Represents a single test app which can be installed and interacted with. */
@TestAppSender
public class TestApp {

    private static final TestApis sTestApis = new TestApis();
    // Must be instrumentation context to access resources
    private static final Context sContext = sTestApis.context().instrumentationContext();
    private final TestAppDetails mDetails;

    TestApp(TestAppDetails details) {
        if (details == null) {
            throw new NullPointerException();
        }
        mDetails = details;
    }

    /**
     * Get a {@link PackageReference} for the {@link TestApp}.
     *
     * <p>This will only be resolvable after the app is installed.
     */
    public PackageReference reference() {
        return sTestApis.packages().find(packageName());
    }

    /**
     * Get a {@link Package} for the {@link TestApp}, or {@code null} if it is not installed.
     */
    public Package resolve() {
        return reference().resolve();
    }

    /**
     * Install the {@link TestApp} on the device for the given {@link UserReference}.
     */
    public TestAppInstanceReference install(UserReference user) {
        sTestApis.packages().install(user, apkBytes());
        return new TestAppInstanceReference(this, user);
    }

    /**
     * Install the {@link TestApp} on the device for the given {@link UserHandle}.
     */
    public TestAppInstanceReference install(UserHandle user) {
        install(sTestApis.users().find(user));
        return instance(user);
    }

    /**
     * Uninstall the {@link TestApp} on the device from the given {@link UserReference}.
     */
    public void uninstall(UserReference user) {
        reference().uninstall(user);
    }

    /**
     * Uninstall the {@link TestApp} on the device from the given {@link UserHandle}.
     */
    public void uninstall(UserHandle user) {
        uninstall(sTestApis.users().find(user));
    }

    /**
     * Get a reference to the specific instance of this test app on a given user.
     *
     * <p>This does not check if the user exists, or if the test app is installed on the user.
     */
    public TestAppInstanceReference instance(UserHandle user) {
        return instance(sTestApis.users().find(user));
    }

    /**
     * Get a reference to the specific instance of this test app on a given user.
     *
     * <p>This does not check if the user exists, or if the test app is installed on the user.
     */
    public TestAppInstanceReference instance(UserReference user) {
        if (user == null) {
            throw new NullPointerException();
        }
        return new TestAppInstanceReference(this, user);
    }

    private byte[] apkBytes() {
        try (InputStream inputStream =
                     sContext.getResources().openRawResource(mDetails.mResourceIdentifier)) {
            return readInputStreamFully(inputStream);
        } catch (IOException e) {
            throw new NeneException("Error when reading TestApp bytes", e);
        }
    }

    /** Write the APK file to the given {@link File}. */
    public void writeApkFile(File outputFile) throws IOException {
        try (FileOutputStream output = new FileOutputStream(outputFile)) {
            output.write(apkBytes());
        }
    }

    /** The package name of the test app. */
    public String packageName() {
        return mDetails.mPackageName;
    }
}
