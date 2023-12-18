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
package com.android.compatibility.common.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.DropBoxManager;
import android.util.Log;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class DropBoxReceiver {
    private static final String TAG = "DropBoxReceiver";
    private static final int TIMEOUT_SECS = 60 * 3;

    private CountDownLatch mResultsReceivedSignal;
    private long mStartMs;

    public DropBoxReceiver(Context ctx, String wantTag, String... wantInStackTrace) {
        mResultsReceivedSignal = new CountDownLatch(1);
        mStartMs = System.currentTimeMillis();

        DropBoxManager mDropbox = (DropBoxManager) ctx.getSystemService(Context.DROPBOX_SERVICE);

        ctx.registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        // DropBox might receive other entries while we're waiting for the error
                        // entry, so we need to check the tag and stack trace before continuing.
                        while (true) {
                            final DropBoxManager.Entry entry =
                                    mDropbox.getNextEntry(wantTag, mStartMs);
                            if (entry == null) {
                                break;
                            }
                            Log.d(TAG, "ErrorsTest got message from drobpox: " + entry.getTag());
                            mStartMs = entry.getTimeMillis();
                            String stackTrace = entry.getText(64 * 1024);
                            boolean allMatches = true;
                            for (String line : wantInStackTrace) {
                                boolean matched = stackTrace.contains(line);
                                Log.d(TAG, "   matched=" + matched + " line: " + line);
                                allMatches &= matched;
                            }
                            if (allMatches) {
                                mResultsReceivedSignal.countDown();
                            }
                            entry.close();
                        }
                    }
                },
                new IntentFilter(DropBoxManager.ACTION_DROPBOX_ENTRY_ADDED));
    }

    public boolean await() throws InterruptedException {
        return mResultsReceivedSignal.await(TIMEOUT_SECS, TimeUnit.SECONDS);
    }
}
