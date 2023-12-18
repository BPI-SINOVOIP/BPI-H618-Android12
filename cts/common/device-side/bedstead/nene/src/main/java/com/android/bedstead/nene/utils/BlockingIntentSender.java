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

package com.android.bedstead.nene.utils;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentSender;

import com.android.bedstead.nene.TestApis;
import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import java.util.UUID;

/**
 * Provider of a blocking version of {@link IntentSender}.
 *
 * <p>To use:
 * {@code
 *     try(BlockingIntentSender blockingIntentSender = BlockingIntentSender.create()) {
 *          IntentSender intentSender = blockingIntentSender.intentSender();
 *          // Use the intentSender for something
 *     }
 *     // we will block on the intent sender being used before exiting the try block, an exception
 *     // will be thrown if it is not used
 * }
 */
public class BlockingIntentSender implements AutoCloseable {

    private static final String ACTION_PREFIX = "com.android.bedstead.intentsender.";

    private final TestApis mTestApis = new TestApis();

    /** Create and register a {@link BlockingIntentSender}. */
    public static BlockingIntentSender create() {
        BlockingIntentSender blockingIntentSender =
                new BlockingIntentSender(
                        ACTION_PREFIX + UUID.randomUUID().getLeastSignificantBits());
        blockingIntentSender.register();

        return blockingIntentSender;
    }

    private final String mAction;
    private IntentSender mIntentSender;
    private BlockingBroadcastReceiver mBlockingBroadcastReceiver;

    private BlockingIntentSender(String action) {
        mAction = action;
    }

    private void register() {
        mBlockingBroadcastReceiver = BlockingBroadcastReceiver.create(
                mTestApis.context().instrumentedContext(), mAction);
        mBlockingBroadcastReceiver.register();

        Intent intent = new Intent(mAction);
        intent.setPackage(mTestApis.context().instrumentedContext().getPackageName());
        PendingIntent pendingIntent = PendingIntent.getBroadcast(
                mTestApis.context().instrumentedContext(), 0, intent, /* flags= */ 0);
        mIntentSender = pendingIntent.getIntentSender();
    }

    /** Wait for the {@link #intentSender()} to be used. */
    public Intent await() {
        return mBlockingBroadcastReceiver.awaitForBroadcast();
    }

    /** Get the intent sender. */
    public IntentSender intentSender() {
        return mIntentSender;
    }

    @Override
    public void close() {
        mBlockingBroadcastReceiver.close();
    }
}
