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
package android.app.time.cts.shell.host;

import android.app.time.cts.shell.DeviceShellCommandExecutor;

import androidx.annotation.NonNull;

import com.android.tradefed.device.CollectingByteOutputReceiver;
import com.android.tradefed.device.ITestDevice;

import java.util.Objects;

public final class HostShellCommandExecutor extends DeviceShellCommandExecutor {

    private final ITestDevice mDevice;

    public HostShellCommandExecutor(@NonNull ITestDevice device) {
        mDevice = Objects.requireNonNull(device);
    }

    @Override
    @NonNull
    protected byte[] executeToBytesInternal(String command) throws Exception {
        CollectingByteOutputReceiver bytesReceiver = new CollectingByteOutputReceiver();
        mDevice.executeShellCommand(command, bytesReceiver);
        return bytesReceiver.getOutput();
    }

    @Override
    protected void log(String msg) {
        System.out.println("HostShellCommandExecutor: " + msg);
    }
}
