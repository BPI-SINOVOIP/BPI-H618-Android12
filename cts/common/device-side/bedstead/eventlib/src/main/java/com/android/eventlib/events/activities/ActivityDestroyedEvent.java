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
 * Event logged when {@link Activity#onDestroy()} is called.
 */
public final class ActivityDestroyedEvent extends Event {

    /** Begin a query for {@link ActivityDestroyedEvent} events. */
    public static ActivityDestroyedEventQuery queryPackage(String packageName) {
        return new ActivityDestroyedEventQuery(packageName);
    }

    /** {@link EventLogsQuery} for {@link ActivityDestroyedEvent}. */
    public static final class ActivityDestroyedEventQuery
            extends EventLogsQuery<ActivityDestroyedEvent, ActivityDestroyedEventQuery> {
        ActivityQueryHelper<ActivityDestroyedEventQuery> mActivity =
                new ActivityQueryHelper<>(this);

        private ActivityDestroyedEventQuery(String packageName) {
            super(ActivityDestroyedEvent.class, packageName);
        }

        /** Query {@link Activity}. */
        @CheckResult
        public ActivityQuery<ActivityDestroyedEventQuery> whereActivity() {
            return mActivity;
        }

        @Override
        protected boolean filter(ActivityDestroyedEvent event) {
            if (!mActivity.matches(event.mActivity)) {
                return false;
            }
            return true;
        }
    }

    /** Begin logging a {@link ActivityDestroyedEvent}. */
    public static ActivityDestroyedEventLogger logger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
        return new ActivityDestroyedEventLogger(activity, activityInfo);
    }

    /** {@link EventLogger} for {@link ActivityDestroyedEvent}. */
    public static final class ActivityDestroyedEventLogger
            extends EventLogger<ActivityDestroyedEvent> {
        private ActivityDestroyedEventLogger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
            super(activity, new ActivityDestroyedEvent());
            setActivity(activityInfo);
        }

        /** Set the {@link Activity} being destroyed. */
        public ActivityDestroyedEventLogger setActivity(android.content.pm.ActivityInfo activity) {
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
        return "ActivityDestroyedEvent{"
                + ", activity=" + mActivity
                + ", packageName='" + mPackageName + "'"
                + ", timestamp=" + mTimestamp
                + "}";
    }
}
