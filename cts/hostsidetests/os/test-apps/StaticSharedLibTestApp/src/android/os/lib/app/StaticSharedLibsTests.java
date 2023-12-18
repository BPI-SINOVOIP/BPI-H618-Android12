/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.os.lib.app;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;

import static com.google.common.truth.Truth.assertThat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.content.pm.SharedLibraryInfo;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.cts.install.lib.Install;
import com.android.cts.install.lib.InstallUtils;
import com.android.cts.install.lib.TestApp;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Optional;

/**
 * On-device tests driven by StaticSharedLibsHostTests.
 */
@RunWith(AndroidJUnit4.class)
public class StaticSharedLibsTests {

    private static final String STATIC_LIB_PROVIDER5_PKG = "android.os.lib.provider";
    private static final String STATIC_LIB_PROVIDER5_NAME = "android.os.lib.provider_2";
    private static final TestApp TESTAPP_STATIC_LIB_PROVIDER5 = new TestApp(
            "TestStaticSharedLibProvider5", STATIC_LIB_PROVIDER5_PKG, 1, /*isApex*/ false,
            "CtsStaticSharedLibProviderApp5.apk");

    @Before
    public void setUp() throws Exception {
        InstrumentationRegistry
                .getInstrumentation()
                .getUiAutomation()
                .adoptShellPermissionIdentity(
                        Manifest.permission.INSTALL_PACKAGES);
        clear();
    }

    @After
    public void tearDown() throws Exception {
        clear();
        InstrumentationRegistry
                .getInstrumentation()
                .getUiAutomation()
                .dropShellPermissionIdentity();
    }

    @Test
    public void testSamegradeStaticSharedLibFail() throws Exception {
        Install.single(TESTAPP_STATIC_LIB_PROVIDER5).commit();
        assertThat(getSharedLibraryInfo(STATIC_LIB_PROVIDER5_NAME)).isNotNull();

        InstallUtils.commitExpectingFailure(AssertionError.class,
                "Packages declaring static-shared libs cannot be updated",
                Install.single(TESTAPP_STATIC_LIB_PROVIDER5));
    }

    private void clear() {
        uninstallSharedLibrary(STATIC_LIB_PROVIDER5_PKG, STATIC_LIB_PROVIDER5_NAME);
    }

    private SharedLibraryInfo getSharedLibraryInfo(String libName) {
        final PackageManager packageManager = InstrumentationRegistry.getContext()
                .getPackageManager();
        final Optional<SharedLibraryInfo> libraryInfo = packageManager.getSharedLibraries(0)
                .stream().filter(lib -> lib.getName().equals(libName)).findAny();
        return libraryInfo.isPresent() ? libraryInfo.get() : null;
    }

    private void uninstallSharedLibrary(String packageName, String libName) {
        if (getSharedLibraryInfo(libName) == null) {
            return;
        }
        runShellCommand("pm uninstall " + packageName);
        assertThat(getSharedLibraryInfo(libName)).isNull();
    }
}
