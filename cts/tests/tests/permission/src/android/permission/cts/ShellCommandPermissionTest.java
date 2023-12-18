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

package android.permission.cts;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import androidx.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * Test shell command capability enforcement
 */
@RunWith(AndroidJUnit4.class)
public class ShellCommandPermissionTest {

    private void executeShellCommandAndRequireError(String command, String required)
            throws Exception {
        try {
            java.lang.Process proc = Runtime.getRuntime().exec(command);

            // read output of command
            BufferedReader reader =
                    new BufferedReader(new InputStreamReader(proc.getErrorStream()));
            StringBuilder stderr = new StringBuilder();
            String line = reader.readLine();
            while (line != null) {
                stderr.append(line);
                line = reader.readLine();
            }
            reader.close();
            proc.waitFor();

            String stderrString = stderr.toString();
            assertTrue("Expected command stderr '" + stderrString
                            + " to contain '" + required + "'",
                    stderrString.contains(required));
        } catch (InterruptedException e) {
            fail("Unsuccessful shell command");
        }
    }

    @Test
    public void testTraceIpc() throws Exception {
        executeShellCommandAndRequireError(
                "cmd activity trace-ipc stop --dump-file /data/system/last-fstrim",
                "Error:");
    }

    @Test
    public void testDumpheap() throws Exception {
        executeShellCommandAndRequireError(
                "cmd activity dumpheap system_server /data/system/last-fstrim",
                "Error:");
    }
}
