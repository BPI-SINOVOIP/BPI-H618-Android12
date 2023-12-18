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

package android.telephony.ims.cts;

import android.telephony.ims.stub.ImsRegistrationImplBase;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public class TestImsRegistration extends ImsRegistrationImplBase {

    public static class NetworkRegistrationInfo {
        public final int sipCode;
        public final String sipReason;
        NetworkRegistrationInfo(int code, String reason) {
            sipCode = code;
            sipReason = reason;
        }
    }

    public static final int LATCH_UPDATE_REGISTRATION = 0;
    public static final int LATCH_TRIGGER_DEREGISTRATION = 1;
    private static final int LATCH_MAX = 2;
    private static final CountDownLatch[] sLatches = new CountDownLatch[LATCH_MAX];
    static {
        for (int i = 0; i < LATCH_MAX; i++) {
            sLatches[i] = new CountDownLatch(1);
        }
    }

    private final LinkedBlockingQueue<NetworkRegistrationInfo> mPendingFullRegistrationRequests =
            new LinkedBlockingQueue<>();

    @Override
    public void triggerFullNetworkRegistration(int sipCode, String sipReason) {
        mPendingFullRegistrationRequests.offer(new NetworkRegistrationInfo(sipCode, sipReason));
    }

    @Override
    public void updateSipDelegateRegistration() {
        synchronized (sLatches) {
            sLatches[LATCH_UPDATE_REGISTRATION].countDown();
        }
    }

    @Override
    public void triggerSipDelegateDeregistration() {
        synchronized (sLatches) {
            sLatches[LATCH_TRIGGER_DEREGISTRATION].countDown();
        }
    }

    public NetworkRegistrationInfo getNextFullNetworkRegRequest(int timeoutMs) throws Exception {
        return mPendingFullRegistrationRequests.poll(timeoutMs, TimeUnit.MILLISECONDS);
    }

    public void resetLatch(int latchIndex, int newCount) {
        synchronized (sLatches) {
            sLatches[latchIndex] = new CountDownLatch(newCount);
        }
    }

    public boolean waitForLatchCountDown(int latchIndex, int timeoutMs) {
        CountDownLatch latch;
        synchronized (sLatches) {
            latch = sLatches[latchIndex];
        }
        while (latch.getCount() > 0) {
            try {
                return latch.await(timeoutMs, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) { }
        }
        return true;
    }
}
