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

package com.android.eventlib.events.broadcastreceivers;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.CheckResult;

import com.android.eventlib.Event;
import com.android.eventlib.EventLogger;
import com.android.eventlib.EventLogsQuery;
import com.android.queryable.info.BroadcastReceiverInfo;
import com.android.queryable.queries.BroadcastReceiverQuery;
import com.android.queryable.queries.BroadcastReceiverQueryHelper;
import com.android.queryable.queries.IntentQuery;
import com.android.queryable.queries.IntentQueryHelper;
import com.android.queryable.util.SerializableParcelWrapper;

/**
 * Event logged when {@link BroadcastReceiver#onReceive(Context, Intent)} is called.
 */
public final class BroadcastReceivedEvent extends Event {

    /** Begin a query for {@link BroadcastReceivedEvent} events. */
    public static BroadcastReceivedEventQuery queryPackage(String packageName) {
        return new BroadcastReceivedEventQuery(packageName);
    }

    /** {@link EventLogsQuery} for {@link BroadcastReceivedEvent}. */
    public static final class BroadcastReceivedEventQuery
            extends EventLogsQuery<BroadcastReceivedEvent, BroadcastReceivedEventQuery> {
        BroadcastReceiverQueryHelper<BroadcastReceivedEventQuery> mBroadcastReceiver =
                new BroadcastReceiverQueryHelper<>(this);
        IntentQueryHelper<BroadcastReceivedEventQuery> mIntent = new IntentQueryHelper<>(this);

        private BroadcastReceivedEventQuery(String packageName) {
            super(BroadcastReceivedEvent.class, packageName);
        }

        /**
         * Query {@link Intent} passed into {@link BroadcastReceiver#onReceive(Context, Intent)}.
         */
        @CheckResult
        public IntentQuery<BroadcastReceivedEventQuery> whereIntent() {
            return mIntent;
        }

        /** Query {@link BroadcastReceiver}. */
        @CheckResult
        public BroadcastReceiverQuery<BroadcastReceivedEventQuery> whereBroadcastReceiver() {
            return mBroadcastReceiver;
        }

        @Override
        protected boolean filter(BroadcastReceivedEvent event) {
            if (!mIntent.matches(event.mIntent)) {
                return false;
            }
            if (!mBroadcastReceiver.matches(event.mBroadcastReceiver)) {
                return false;
            }
            return true;
        }
    }

    /** Begin logging a {@link BroadcastReceivedEvent}. */
    public static BroadcastReceivedEventLogger logger(
            BroadcastReceiver broadcastReceiver, Context context, Intent intent) {
        return new BroadcastReceivedEventLogger(broadcastReceiver, context, intent);
    }

    /** {@link EventLogger} for {@link BroadcastReceivedEvent}. */
    public static final class BroadcastReceivedEventLogger
            extends EventLogger<BroadcastReceivedEvent> {
        private BroadcastReceivedEventLogger(
                BroadcastReceiver broadcastReceiver, Context context, Intent intent) {
            super(context, new BroadcastReceivedEvent());
            mEvent.mIntent = new SerializableParcelWrapper<>(intent);
            setBroadcastReceiver(broadcastReceiver);
        }

        /** Set the {@link BroadcastReceiver} which received this event. */
        public BroadcastReceivedEventLogger setBroadcastReceiver(
                BroadcastReceiver broadcastReceiver) {
            mEvent.mBroadcastReceiver = new BroadcastReceiverInfo(broadcastReceiver);
            return this;
        }

        /** Set the {@link BroadcastReceiver} which received this event. */
        public BroadcastReceivedEventLogger setBroadcastReceiver(
                Class<? extends BroadcastReceiver> broadcastReceiverClass) {
            mEvent.mBroadcastReceiver = new BroadcastReceiverInfo(broadcastReceiverClass);
            return this;
        }

        /** Set the {@link BroadcastReceiver} which received this event. */
        public BroadcastReceivedEventLogger setBroadcastReceiver(
                String broadcastReceiverClassName) {
            mEvent.mBroadcastReceiver = new BroadcastReceiverInfo(broadcastReceiverClassName);
            return this;
        }

        /** Set the {@link Intent} which was received. */
        public BroadcastReceivedEventLogger setIntent(Intent intent) {
            mEvent.mIntent = new SerializableParcelWrapper<>(intent);
            return this;
        }
    }

    protected SerializableParcelWrapper<Intent> mIntent;
    protected BroadcastReceiverInfo mBroadcastReceiver;

    /**
     * The {@link Intent} passed into {@link BroadcastReceiver#onReceive(Context, Intent)}.
     */
    public Intent intent() {
        if (mIntent == null) {
            return null;
        }
        return mIntent.get();
    }

    /** Information about the {@link BroadcastReceiver} which received the intent. */
    public BroadcastReceiverInfo broadcastReceiver() {
        return mBroadcastReceiver;
    }

    @Override
    public String toString() {
        return "BroadcastReceivedEvent{"
                + " intent=" + intent()
                + ", broadcastReceiver=" + mBroadcastReceiver
                + ", packageName='" + mPackageName + "'"
                + ", timestamp=" + mTimestamp
                + "}";
    }
}
