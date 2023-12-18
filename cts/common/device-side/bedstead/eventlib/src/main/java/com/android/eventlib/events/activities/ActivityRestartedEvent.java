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
 * Event logged when {@link Activity#onRestart()} is called.
 */
public final class ActivityRestartedEvent extends Event {

    /** Begin a query for {@link ActivityRestartedEvent} events. */
    public static ActivityRestartedEventQuery queryPackage(String packageName) {
        return new ActivityRestartedEventQuery(packageName);
    }

    /** {@link EventLogsQuery} for {@link ActivityRestartedEvent}. */
    public static final class ActivityRestartedEventQuery
            extends EventLogsQuery<ActivityRestartedEvent, ActivityRestartedEventQuery> {
        ActivityQueryHelper<ActivityRestartedEventQuery> mActivity =
                new ActivityQueryHelper<>(this);

        private ActivityRestartedEventQuery(String packageName) {
            super(ActivityRestartedEvent.class, packageName);
        }

        /** Query {@link Activity}. */
        @CheckResult
        public ActivityQuery<ActivityRestartedEventQuery> whereActivity() {
            return mActivity;
        }

        @Override
        protected boolean filter(ActivityRestartedEvent event) {
            if (!mActivity.matches(event.mActivity)) {
                return false;
            }
            return true;
        }
    }

    /** Begin logging a {@link ActivityRestartedEvent}. */
    public static ActivityRestartedEventLogger logger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
        return new ActivityRestartedEventLogger(activity, activityInfo);
    }

    /** {@link EventLogger} for {@link ActivityRestartedEvent}. */
    public static final class ActivityRestartedEventLogger
            extends EventLogger<ActivityRestartedEvent> {
        private ActivityRestartedEventLogger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
            super(activity, new ActivityRestartedEvent());
            setActivity(activityInfo);
        }

        /** Set the {@link Activity} being destroyed. */
        public ActivityRestartedEventLogger setActivity(android.content.pm.ActivityInfo activity) {
            mEvent.mActivity = ActivityInfo.builder(activity).build();
            return this;
        }
    }

    protected ActivityInfo mActivity;

    /** Information about the {@link Activity} destroyed. */
    public ActivityInfo activity() {
        return mActivity;
    }

    @Override
    public String toString() {
        return "ActivityRestartedEvent{"
                + ", activity=" + mActivity
                + ", packageName='" + mPackageName + "'"
                + ", timestamp=" + mTimestamp
                + "}";
    }
}
