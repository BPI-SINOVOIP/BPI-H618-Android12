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

package android.scopedstorage.cts.host;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import android.platform.test.annotations.AppModeFull;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.contentprovider.ContentProviderHandler;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.util.CommandResult;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Runs the AppCloning tests.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
@AppModeFull
public class AppCloningHostTest extends BaseHostTestCase {
    private static final String APP_A = "CtsScopedStorageTestAppA.apk";
    private static final String APP_A_PACKAGE = "android.scopedstorage.cts.testapp.A.withres";
    private static final String CONTENT_PROVIDER_URL = "content://android.tradefed.contentprovider";
    private static final int CLONE_PROFILE_DIRECTORY_CREATION_TIMEOUT_MS = 20000;
    private String mCloneUserId;
    private ContentProviderHandler mContentProviderHandler;


    @Before
    public void setup() throws Exception {
        assumeFalse("Device is in headless system user mode", isHeadlessSystemUserMode());
        assumeTrue(isAtLeastS());

        String output = executeShellCommand(
                "pm create-user --profileOf 0 --user-type android.os.usertype.profile.CLONE "
                        + "testUser");
        mCloneUserId = output.substring(output.lastIndexOf(' ') + 1).replaceAll("[^0-9]", "");
        assertThat(mCloneUserId).isNotEmpty();
        CommandResult out = executeShellV2Command("am start-user -w %s", mCloneUserId);
        assertThat(out.getStderr()).isEmpty();
        mContentProviderHandler = new ContentProviderHandler(getDevice());
        mContentProviderHandler.setUp();
    }

    @After
    public void tearDown() throws Exception {
        if (isHeadlessSystemUserMode() || !isAtLeastS()) return;
        mContentProviderHandler.tearDown();
        executeShellCommand("pm remove-user %s", mCloneUserId);
    }

    @Test
    public void testInstallAppTwice() throws Exception {
        installAppAsUser(APP_A, getCurrentUserId());
        installAppAsUser(APP_A, Integer.valueOf(mCloneUserId));
        uninstallPackage(APP_A_PACKAGE);
    }

    @Test
    public void testCreateCloneUserFile() throws Exception {
        CommandResult out;

        // Check that the clone user directories exist
        eventually(() -> {
            // Wait for finish.
            assertThat(isSuccessful(
                    runContentProviderCommand("query", mCloneUserId, "/sdcard", ""))).isTrue();
        }, CLONE_PROFILE_DIRECTORY_CREATION_TIMEOUT_MS);

        // Create a file on the clone user storage
        out = executeShellV2Command("touch /sdcard/testFile.txt");
        assertThat(isSuccessful(out)).isTrue();
        eventually(() -> {
            // Wait for finish.
            assertThat(isSuccessful(
                    runContentProviderCommand("write", mCloneUserId, "/sdcard/testFile.txt",
                            "< /sdcard/testFile.txt"))).isTrue();
        }, CLONE_PROFILE_DIRECTORY_CREATION_TIMEOUT_MS);

        // Check that the above created file exists on the clone user storage
        out = runContentProviderCommand("query", mCloneUserId, "/sdcard/testFile.txt", "");
        assertThat(isSuccessful(out)).isTrue();

        // Cleanup the created file
        out = runContentProviderCommand("delete", mCloneUserId, "/sdcard/testFile.txt", "");
        assertThat(isSuccessful(out)).isTrue();
    }

    @Test
    public void testPrivateAppDataDirectoryForCloneUser() throws Exception {
        installAppAsUser(APP_A, Integer.valueOf(mCloneUserId));
        eventually(() -> {
            // Wait for finish.
            assertThat(isPackageInstalled(APP_A_PACKAGE, mCloneUserId)).isTrue();
        }, CLONE_PROFILE_DIRECTORY_CREATION_TIMEOUT_MS);
    }

    private void installAppAsUser(String packageFile, int userId)
            throws TargetSetupError, DeviceNotAvailableException {
        installPackageAsUser(packageFile, false, userId, "-t");
    }

    private CommandResult runContentProviderCommand(String commandType, String userId,
            String relativePath, String args) throws Exception {
        String fullUri = CONTENT_PROVIDER_URL + relativePath;
        return executeShellV2Command("content %s --user %s --uri %s %s",
                commandType, userId, fullUri, args);
    }

}
