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

package com.android.eventlib.events.activities;

import android.app.Activity;

import androidx.annotation.CheckResult;

import com.android.eventlib.Event;
import com.android.eventlib.EventLogger;
import com.android.eventlib.EventLogsQuery;
import com.android.queryable.info.ActivityInfo;
import com.android.queryable.queries.ActivityQuery;
import com.android.queryable.queries.ActivityQueryHelper;

/**
 * Event logged when {@link Activity#onStart()} is called.
 */
public final class ActivityStartedEvent extends Event {

    /** Begin a query for {@link ActivityStartedEvent} events. */
    public static ActivityStartedEventQuery queryPackage(String packageName) {
        return new ActivityStartedEventQuery(packageName);
    }

    /** {@link EventLogsQuery} for {@link ActivityStartedEvent}. */
    public static final class ActivityStartedEventQuery
            extends EventLogsQuery<ActivityStartedEvent, ActivityStartedEventQuery> {
        ActivityQueryHelper<ActivityStartedEventQuery> mActivity = new ActivityQueryHelper<>(this);

        private ActivityStartedEventQuery(String packageName) {
            super(ActivityStartedEvent.class, packageName);
        }

        /** Query {@link Activity}. */
        @CheckResult
        public ActivityQuery<ActivityStartedEventQuery> whereActivity() {
            return mActivity;
        }

        @Override
        protected boolean filter(ActivityStartedEvent event) {
            if (!mActivity.matches(event.mActivity)) {
                return false;
            }
            return true;
        }
    }

    /** Begin logging a {@link ActivityStartedEvent}. */
    public static ActivityStartedEventLogger logger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
        return new ActivityStartedEventLogger(activity, activityInfo);
    }

    /** {@link EventLogger} for {@link ActivityStartedEvent}. */
    public static final class ActivityStartedEventLogger extends EventLogger<ActivityStartedEvent> {
        private ActivityStartedEventLogger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
            super(activity, new ActivityStartedEvent());
            setActivity(activityInfo);
        }

        /** Set the {@link Activity} being started. */
        public ActivityStartedEventLogger setActivity(
                android.content.pm.ActivityInfo activity) {
            mEvent.mActivity = ActivityInfo.builder(activity).build();
            return this;
        }
    }

    protected ActivityInfo mActivity;

    /** Information about the {@link Activity} started. */
    public ActivityInfo activity() {
        return mActivity;
    }

    @Override
    public String toString() {
        return "ActivityStartedEvent{"
                + ", activity=" + mActivity
                + ", packageName='" + mPackageName + "'"
                + ", timestamp=" + mTimestamp
                + "}";
    }
}
