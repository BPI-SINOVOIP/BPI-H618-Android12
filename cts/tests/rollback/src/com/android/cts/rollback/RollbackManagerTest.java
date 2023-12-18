/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.android.cts.rollback;

import static com.android.cts.rollback.lib.RollbackInfoSubject.assertThat;
import static com.android.cts.rollback.lib.RollbackUtils.getRollbackManager;

import static com.google.common.truth.Truth.assertThat;

import android.Manifest;
import android.content.rollback.RollbackInfo;
import android.provider.DeviceConfig;

import androidx.test.InstrumentationRegistry;

import com.android.cts.install.lib.Install;
import com.android.cts.install.lib.InstallUtils;
import com.android.cts.install.lib.TestApp;
import com.android.cts.install.lib.Uninstall;
import com.android.cts.rollback.lib.Rollback;
import com.android.cts.rollback.lib.RollbackUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.IOException;

/**
 * CTS Tests for RollbackManager APIs.
 */
@RunWith(JUnit4.class)
public class RollbackManagerTest {

    /**
     * Adopts common permissions needed to test rollbacks and uninstalls the
     * test apps.
     */
    @Before
    public void setup() throws InterruptedException, IOException {
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .adoptShellPermissionIdentity(
                    Manifest.permission.INSTALL_PACKAGES,
                    Manifest.permission.DELETE_PACKAGES,
                    Manifest.permission.TEST_MANAGE_ROLLBACKS,
                    Manifest.permission.READ_DEVICE_CONFIG,
                    Manifest.permission.WRITE_DEVICE_CONFIG);

        Uninstall.packages(TestApp.A);
    }

    /**
     * Drops adopted shell permissions and uninstalls the test apps.
     */
    @After
    public void teardown() throws InterruptedException, IOException {
        Uninstall.packages(TestApp.A);

        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .dropShellPermissionIdentity();
    }

    /**
     * Test basic rollbacks.
     */
    @Test
    public void testBasic() throws Exception {
        Install.single(TestApp.A1).commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);
        RollbackUtils.waitForRollbackGone(
                () -> getRollbackManager().getAvailableRollbacks(), TestApp.A);
        RollbackUtils.waitForRollbackGone(
                () -> getRollbackManager().getRecentlyCommittedRollbacks(), TestApp.A);

        Install.single(TestApp.A2).setEnableRollback().commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(2);
        InstallUtils.processUserData(TestApp.A);
        RollbackInfo available = RollbackUtils.waitForAvailableRollback(TestApp.A);
        assertThat(available).isNotStaged();
        assertThat(available).packagesContainsExactly(
                Rollback.from(TestApp.A2).to(TestApp.A1));
        assertThat(RollbackUtils.getCommittedRollback(TestApp.A)).isNull();

        RollbackUtils.rollback(available.getRollbackId(), TestApp.A2);
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);
        InstallUtils.processUserData(TestApp.A);
        RollbackUtils.waitForUnavailableRollback(TestApp.A);
        RollbackInfo committed = RollbackUtils.getCommittedRollback(TestApp.A);
        assertThat(committed).isNotNull();
        assertThat(committed).hasRollbackId(available.getRollbackId());
        assertThat(committed).isNotStaged();
        assertThat(committed).packagesContainsExactly(
                Rollback.from(TestApp.A2).to(TestApp.A1));
        assertThat(committed).causePackagesContainsExactly(TestApp.A2);
    }

    @Test
    public void testGetRollbackDataPolicy() throws Exception {
        // TODO: To change to the following statement when
        // PackageManager.RollbackDataPolicy.WIPE is available.
        // final int rollBackDataPolicy = PackageManager.RollbackDataPolicy.WIPE;
        final int rollBackDataPolicy = 1;

        Install.single(TestApp.A1).commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);

        // Enable rollback with rollBackDataPolicy
        final int sessionId = Install.single(TestApp.A2).setEnableRollback(
                rollBackDataPolicy).createSession();

        try {
            assertThat(InstallUtils.getPackageInstaller().getSessionInfo(
                    sessionId).getRollbackDataPolicy()).isEqualTo(rollBackDataPolicy);
        } finally {
            // Abandon the session
            InstallUtils.getPackageInstaller().abandonSession(sessionId);
        }
    }

    /**
     * Test that flags are cleared when a rollback is committed.
     */
    @Test
    public void testRollbackClearsFlags() throws Exception {
        Install.single(TestApp.A1).commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);
        RollbackUtils.waitForRollbackGone(
                () -> getRollbackManager().getAvailableRollbacks(), TestApp.A);

        Install.single(TestApp.A2).setEnableRollback().commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(2);
        RollbackInfo available = RollbackUtils.waitForAvailableRollback(TestApp.A);

        DeviceConfig.setProperty("configuration", "namespace_to_package_mapping",
                "testspace:" + TestApp.A, false);
        DeviceConfig.setProperty("testspace", "flagname", "hello", false);
        DeviceConfig.setProperty("testspace", "another", "12345", false);
        assertThat(DeviceConfig.getProperties("testspace").getKeyset()).hasSize(2);

        RollbackUtils.rollback(available.getRollbackId(), TestApp.A2);
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);

        assertThat(DeviceConfig.getProperties("testspace").getKeyset()).hasSize(0);
    }

    /**
     * Tests an app can be rolled back to the previous signing key.
     *
     * <p>The rollback capability in the signing lineage allows an app to be updated to an APK
     * signed with a previous signing key in the lineage; however this often defeats the purpose
     * of key rotation as a compromised key could then be used to roll an app back to the previous
     * key. To avoid requiring the rollback capability to support APK rollbacks the PackageManager
     * allows an app to be rolled back to the previous signing key if the rollback install reason
     * is set.
     */
    @Test
    public void testRollbackAfterKeyRotation() throws Exception {
        Install.single(TestApp.AOriginal1).commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);

        Install.single(TestApp.ARotated2).setEnableRollback().commit();
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(2);

        RollbackInfo available = RollbackUtils.waitForAvailableRollback(TestApp.A);
        RollbackUtils.rollback(available.getRollbackId(), TestApp.ARotated2);
        assertThat(InstallUtils.getInstalledVersion(TestApp.A)).isEqualTo(1);
    }
}
