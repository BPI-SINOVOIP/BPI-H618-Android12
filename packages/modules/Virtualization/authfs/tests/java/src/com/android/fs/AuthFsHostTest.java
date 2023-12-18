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

package com.android.virt.fs;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.platform.test.annotations.RootPermissionTest;

import com.android.compatibility.common.util.PollingCheck;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@RootPermissionTest
@RunWith(DeviceJUnit4ClassRunner.class)
public final class AuthFsHostTest extends BaseHostJUnit4Test {

    /** Test directory where data are located */
    private static final String TEST_DIR = "/data/local/tmp/authfs";

    /** Mount point of authfs during the test */
    private static final String MOUNT_DIR = "/data/local/tmp/authfs/mnt";

    private static final String FD_SERVER_BIN = "/apex/com.android.virt/bin/fd_server";
    private static final String AUTHFS_BIN = "/apex/com.android.virt/bin/authfs";

    /** Plenty of time for authfs to get ready */
    private static final int AUTHFS_INIT_TIMEOUT_MS = 1500;

    /** FUSE's magic from statfs(2) */
    private static final String FUSE_SUPER_MAGIC_HEX = "65735546";

    private ITestDevice mDevice;
    private ExecutorService mThreadPool;

    @Before
    public void setUp() {
        mDevice = getDevice();
        mThreadPool = Executors.newCachedThreadPool();
    }

    @After
    public void tearDown() throws DeviceNotAvailableException {
        mDevice.executeShellV2Command("killall authfs fd_server");
        mDevice.executeShellV2Command("umount " + MOUNT_DIR);
        mDevice.executeShellV2Command("rm -f " + TEST_DIR + "/output");
    }

    @Test
    public void testReadWithFsverityVerification_LocalFile()
            throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runAuthFsInBackground(
                "--local-ro-file-unverified 3:input.4m"
                + " --local-ro-file 4:input.4m:input.4m.merkle_dump:input.4m.fsv_sig:cert.der"
                + " --local-ro-file 5:input.4k1:input.4k1.merkle_dump:input.4k1.fsv_sig:cert.der"
                + " --local-ro-file 6:input.4k:input.4k.merkle_dump:input.4k.fsv_sig:cert.der"
        );

        // Action
        String actualHashUnverified4m = computeFileHashInGuest(MOUNT_DIR + "/3");
        String actualHash4m = computeFileHashInGuest(MOUNT_DIR + "/4");
        String actualHash4k1 = computeFileHashInGuest(MOUNT_DIR + "/5");
        String actualHash4k = computeFileHashInGuest(MOUNT_DIR + "/6");

        // Verify
        String expectedHash4m = computeFileHash(TEST_DIR + "/input.4m");
        String expectedHash4k1 = computeFileHash(TEST_DIR + "/input.4k1");
        String expectedHash4k = computeFileHash(TEST_DIR + "/input.4k");

        assertEquals("Inconsistent hash from /authfs/3: ", expectedHash4m, actualHashUnverified4m);
        assertEquals("Inconsistent hash from /authfs/4: ", expectedHash4m, actualHash4m);
        assertEquals("Inconsistent hash from /authfs/5: ", expectedHash4k1, actualHash4k1);
        assertEquals("Inconsistent hash from /authfs/6: ", expectedHash4k, actualHash4k);
    }

    @Test
    public void testReadWithFsverityVerification_RemoteFile()
            throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runFdServerInBackground(
                "3<input.4m 4<input.4m.merkle_dump 5<input.4m.fsv_sig 6<input.4m",
                "--ro-fds 3:4:5 --ro-fds 6"
        );
        runAuthFsInBackground(
                "--remote-ro-file-unverified 10:6:4194304 --remote-ro-file 11:3:4194304:cert.der"
        );

        // Action
        String actualHashUnverified4m = computeFileHashInGuest(MOUNT_DIR + "/10");
        String actualHash4m = computeFileHashInGuest(MOUNT_DIR + "/11");

        // Verify
        String expectedHash4m = computeFileHash(TEST_DIR + "/input.4m");

        assertEquals("Inconsistent hash from /authfs/10: ", expectedHash4m, actualHashUnverified4m);
        assertEquals("Inconsistent hash from /authfs/11: ", expectedHash4m, actualHash4m);
    }

    // Separate the test from the above simply because exec in shell does not allow open too many
    // files.
    @Test
    public void testReadWithFsverityVerification_RemoteSmallerFile()
            throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runFdServerInBackground(
                "3<input.4k 4<input.4k.merkle_dump 5<input.4k.fsv_sig"
                + " 6<input.4k1 7<input.4k1.merkle_dump 8<input.4k1.fsv_sig",
                "--ro-fds 3:4:5 --ro-fds 6:7:8"
        );
        runAuthFsInBackground(
                "--remote-ro-file 10:3:4096:cert.der --remote-ro-file 11:6:4097:cert.der"
        );

        // Action
        String actualHash4k = computeFileHashInGuest(MOUNT_DIR + "/10");
        String actualHash4k1 = computeFileHashInGuest(MOUNT_DIR + "/11");

        // Verify
        String expectedHash4k = computeFileHash(TEST_DIR + "/input.4k");
        String expectedHash4k1 = computeFileHash(TEST_DIR + "/input.4k1");

        assertEquals("Inconsistent hash from /authfs/10: ", expectedHash4k, actualHash4k);
        assertEquals("Inconsistent hash from /authfs/11: ", expectedHash4k1, actualHash4k1);
    }

    @Test
    public void testReadWithFsverityVerification_TamperedMerkleTree()
            throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runFdServerInBackground(
                "3<input.4m 4<input.4m.merkle_dump.bad 5<input.4m.fsv_sig",
                "--ro-fds 3:4:5"
        );
        runAuthFsInBackground("--remote-ro-file 10:3:4096:cert.der");

        // Verify
        assertFalse(copyFileInGuest(MOUNT_DIR + "/10", "/dev/null"));
    }

    @Test
    public void testWriteThroughCorrectly()
            throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runFdServerInBackground("3<>output", "--rw-fds 3");
        runAuthFsInBackground("--remote-new-rw-file 20:3");

        // Action
        String srcPath = "/system/bin/linker";
        String destPath = MOUNT_DIR + "/20";
        String backendPath = TEST_DIR + "/output";
        assertTrue(copyFileInGuest(srcPath, destPath));

        // Verify
        String expectedHash = computeFileHashInGuest(srcPath);
        expectBackingFileConsistency(destPath, backendPath, expectedHash);
    }

    @Test
    public void testWriteFailedIfDetectsTampering()
            throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runFdServerInBackground("3<>output", "--rw-fds 3");
        runAuthFsInBackground("--remote-new-rw-file 20:3");

        String srcPath = "/system/bin/linker";
        String destPath = MOUNT_DIR + "/20";
        String backendPath = TEST_DIR + "/output";
        assertTrue(copyFileInGuest(srcPath, destPath));

        // Action
        // Tampering with the first 2 4K block of the backing file.
        expectRemoteCommandToSucceed("dd if=/dev/zero of=" + backendPath + " bs=1 count=8192");

        // Verify
        // Write to a block partially requires a read back to calculate the new hash. It should fail
        // when the content is inconsistent to the known hash. Use direct I/O to avoid simply
        // writing to the filesystem cache.
        expectRemoteCommandToFail("dd if=/dev/zero of=" + destPath + " bs=1 count=1024 direct");

        // A full 4K write does not require to read back, so write can succeed even if the backing
        // block has already been tampered.
        expectRemoteCommandToSucceed(
                "dd if=/dev/zero of=" + destPath + " bs=1 count=4096 skip=4096");

        // Otherwise, a partial write with correct backing file should still succeed.
        expectRemoteCommandToSucceed(
                "dd if=/dev/zero of=" + destPath + " bs=1 count=1024 skip=8192");
    }

    @Test
    public void testFileResize() throws DeviceNotAvailableException, InterruptedException {
        // Setup
        runFdServerInBackground("3<>output", "--rw-fds 3");
        runAuthFsInBackground("--remote-new-rw-file 20:3");
        String outputPath = MOUNT_DIR + "/20";
        String backendPath = TEST_DIR + "/output";

        // Action & Verify
        expectRemoteCommandToSucceed(
                "yes $'\\x01' | tr -d '\\n' | dd bs=1 count=10000 of=" + outputPath);
        assertEquals(getFileSizeInBytes(outputPath), 10000);
        expectBackingFileConsistency(
                outputPath,
                backendPath,
                "684ad25fdc2bbb80cbc910dd1bde6d5499ccf860ca6ee44704b77ec445271353");

        resizeFile(outputPath, 15000);
        assertEquals(getFileSizeInBytes(outputPath), 15000);
        expectBackingFileConsistency(
                outputPath,
                backendPath,
                "567c89f62586e0d33369157afdfe99a2fa36cdffb01e91dcdc0b7355262d610d");

        resizeFile(outputPath, 5000);
        assertEquals(getFileSizeInBytes(outputPath), 5000);
        expectBackingFileConsistency(
                outputPath,
                backendPath,
                "e53130831c13dabff71d5d1797e3aaa467b4b7d32b3b8782c4ff03d76976f2aa");
    }

    private void expectBackingFileConsistency(
            String authFsPath, String backendPath, String expectedHash)
            throws DeviceNotAvailableException {
        String hashOnAuthFs = computeFileHashInGuest(authFsPath);
        assertEquals("File hash is different to expectation", expectedHash, hashOnAuthFs);

        String hashOfBackingFile = computeFileHash(backendPath);
        assertEquals(
                "Inconsistent file hash on the backend storage", hashOnAuthFs, hashOfBackingFile);
    }

    // TODO(b/178874539): This does not really run in the guest VM.  Send the shell command to the
    // guest VM when authfs works across VM boundary.
    private String computeFileHashInGuest(String path) throws DeviceNotAvailableException {
        return computeFileHash(path);
    }

    private boolean copyFileInGuest(String src, String dest) throws DeviceNotAvailableException {
        // TODO(b/182576497): cp returns error because close(2) returns ENOSYS in the current authfs
        // implementation. We should probably fix that since programs can expect close(2) return 0.
        String cmd = "cat " + src + " > " + dest;
        CommandResult result = mDevice.executeShellV2Command(cmd);
        return result.getStatus() == CommandStatus.SUCCESS;
    }

    private String computeFileHash(String path) throws DeviceNotAvailableException {
        String result = expectRemoteCommandToSucceed("sha256sum " + path);
        String[] tokens = result.split("\\s");
        if (tokens.length > 0) {
            return tokens[0];
        } else {
            CLog.e("Unrecognized output by sha256sum: " + result);
            return "";
        }
    }

    private void resizeFile(String path, long size) throws DeviceNotAvailableException {
        expectRemoteCommandToSucceed("truncate -c -s " + size + " " + path);
    }

    private long getFileSizeInBytes(String path) throws DeviceNotAvailableException {
        return Long.parseLong(expectRemoteCommandToSucceed("stat -c '%s' " + path));
    }

    private void throwDowncastedException(Exception e) throws DeviceNotAvailableException {
        if (e instanceof DeviceNotAvailableException) {
            throw (DeviceNotAvailableException) e;
        } else {
            // Convert the broad Exception into an unchecked exception to avoid polluting all other
            // methods. waitFor throws Exception because the callback, Callable#call(), has a
            // signature to throw an Exception.
            throw new RuntimeException(e);
        }
    }

    private void runAuthFsInBackground(String flags) throws DeviceNotAvailableException {
        String cmd = "cd " + TEST_DIR + " && " + AUTHFS_BIN + " " + MOUNT_DIR + " " + flags;

        mThreadPool.submit(() -> {
            try {
                CLog.i("Starting authfs");
                expectRemoteCommandToSucceed(cmd);
            } catch (DeviceNotAvailableException e) {
                CLog.e("Error running authfs", e);
                throw new RuntimeException(e);
            }
        });
        try {
            PollingCheck.waitFor(AUTHFS_INIT_TIMEOUT_MS, () -> isRemoteDirectoryOnFuse(MOUNT_DIR));
        } catch (Exception e) {
            throwDowncastedException(e);
        }
    }

    private void runFdServerInBackground(String execParamsForOpeningFds, String flags)
            throws DeviceNotAvailableException {
        String cmd = "cd " + TEST_DIR + " && exec " + execParamsForOpeningFds + " " + FD_SERVER_BIN
                + " " + flags;
        mThreadPool.submit(() -> {
            try {
                CLog.i("Starting fd_server");
                expectRemoteCommandToSucceed(cmd);
            } catch (DeviceNotAvailableException e) {
                CLog.e("Error running fd_server", e);
                throw new RuntimeException(e);
            }
        });
    }

    private boolean isRemoteDirectoryOnFuse(String path) throws DeviceNotAvailableException {
        String fs_type = expectRemoteCommandToSucceed("stat -f -c '%t' " + path);
        return FUSE_SUPER_MAGIC_HEX.equals(fs_type);
    }

    private String expectRemoteCommandToSucceed(String cmd) throws DeviceNotAvailableException {
        CommandResult result = mDevice.executeShellV2Command(cmd);
        assertEquals("`" + cmd + "` failed: " + result.getStderr(), CommandStatus.SUCCESS,
                result.getStatus());
        CLog.d("Stdout: " + result.getStdout());
        return result.getStdout().trim();
    }

    private void expectRemoteCommandToFail(String cmd) throws DeviceNotAvailableException {
        CommandResult result = mDevice.executeShellV2Command(cmd);
        assertNotEquals("Unexpected success from `" + cmd + "`: " + result.getStdout(),
                result.getStatus(), CommandStatus.SUCCESS);
    }
}
