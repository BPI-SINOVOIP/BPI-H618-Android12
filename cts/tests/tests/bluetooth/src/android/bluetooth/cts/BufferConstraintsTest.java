/*
 * Copyright 2021 The Android Open Source Project
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
package android.bluetooth.cts;

import android.bluetooth.BluetoothCodecConfig;
import android.bluetooth.BufferConstraint;
import android.bluetooth.BufferConstraints;
import android.content.Context;
import android.content.pm.PackageManager;
import android.test.AndroidTestCase;

import java.util.ArrayList;
import java.util.List;

public class BufferConstraintsTest extends AndroidTestCase {
    private BufferConstraints mBufferConstraints;
    private List<BufferConstraint> mBufferConstraintList;
    private boolean mHasBluetooth;
    private static final int DEFAULT_BUFFER_TIME = 1;
    private static final int MAXIMUM_BUFFER_TIME = 2;
    private static final int MINIMUM_BUFFER_TIME = 3;

    public void setUp() throws Exception {
        super.setUp();
        mHasBluetooth = getContext().getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_BLUETOOTH);
    }

    public void tearDown() throws Exception {
        super.tearDown();
    }

    public void test_forCodec() {
        if (!mHasBluetooth) {
            // Skip the test if bluetooth is not present.
            return;
        }

        mBufferConstraintList = new ArrayList<BufferConstraint>();
        for (int i = 0; i < BufferConstraints.BUFFER_CODEC_MAX_NUM; i++) {
            int defaultBufferTime = DEFAULT_BUFFER_TIME;
            int maximumBufferTime = MAXIMUM_BUFFER_TIME;
            int minimumBufferTime = MINIMUM_BUFFER_TIME;
            BufferConstraint bufferConstraint = new BufferConstraint(defaultBufferTime,
                    maximumBufferTime, minimumBufferTime);
            mBufferConstraintList.add(bufferConstraint);
        }
        mBufferConstraints = new BufferConstraints(mBufferConstraintList);

        for (int i = 0; i < BluetoothCodecConfig.SOURCE_CODEC_TYPE_MAX; i++) {
            assertEquals(DEFAULT_BUFFER_TIME, mBufferConstraints.forCodec(i).getDefaultMillis());
            assertEquals(MAXIMUM_BUFFER_TIME, mBufferConstraints.forCodec(i).getMaxMillis());
            assertEquals(MINIMUM_BUFFER_TIME, mBufferConstraints.forCodec(i).getMinMillis());
        }
    }
}
