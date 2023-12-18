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
package com.android.trout.dumpstate.tests;

import android.hardware.dumpstate.V1_1.DumpstateMode;
import android.hardware.dumpstate.V1_1.DumpstateStatus;
import android.hardware.dumpstate.V1_1.IDumpstateDevice;
import android.os.NativeHandle;
import android.util.Log;
import androidx.test.filters.MediumTest;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import org.apache.commons.compress.archivers.ArchiveStreamFactory;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests used to validate Trout dumpstate E2E functionality.
 */
@RunWith(JUnit4.class)
public class DumpstateE2eTests {
    private static final String TAG = DumpstateE2eTests.class.getSimpleName();
    private IDumpstateDevice mDevice;

    @Rule public TemporaryFolder mTempFolder = new TemporaryFolder();

    @Before
    public void setUp() throws Exception {
        mDevice = android.hardware.dumpstate.V1_1.IDumpstateDevice.getService(true /* retry */);
    }

    @Test
    @MediumTest
    public void testDumpstateBoard() throws Exception {
        File dumpstate_board_txt_file = mTempFolder.newFile();
        File dumpstate_board_bin_file = mTempFolder.newFile();

        try (FileOutputStream dumpstate_board_txt_ostream =
                        new FileOutputStream(dumpstate_board_txt_file.getPath());
                FileOutputStream dumpstate_board_bin_ostream =
                        new FileOutputStream(dumpstate_board_bin_file.getPath());) {
            NativeHandle handle = new NativeHandle(
                    new FileDescriptor[] {
                            dumpstate_board_txt_ostream.getFD(),
                            dumpstate_board_bin_ostream.getFD(),
                    },
                    new int[0], false);
            int dumping_status = mDevice.dumpstateBoard_1_1(
                    handle, DumpstateMode.DEFAULT, 10 * 1000 /* milliseconds */);
            Assert.assertEquals(DumpstateStatus.OK, dumping_status);
        }

        Assert.assertNotEquals(0, dumpstate_board_txt_file.length());
        Assert.assertNotEquals(0, dumpstate_board_bin_file.length());

        try (FileInputStream dumpstate_board_tar_istream =
                        new FileInputStream(dumpstate_board_bin_file.getPath());
                TarArchiveInputStream tar_stream =
                        (TarArchiveInputStream) new ArchiveStreamFactory().createArchiveInputStream(
                                "tar", dumpstate_board_tar_istream);) {
            // Traversing the tar package to make sure it is valid
            TarArchiveEntry tar_entry = null;
            while ((tar_entry = (TarArchiveEntry) tar_stream.getNextEntry()) != null) {
                Log.d(TAG, tar_entry.getName());
            }
        }
    }
}
