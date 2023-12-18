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
 * Event logged when {@link Activity#onPause()} is called.
 */
public final class ActivityPausedEvent extends Event {

    /** Begin a query for {@link ActivityPausedEvent} events. */
    public static ActivityPausedEventQuery queryPackage(String packageName) {
        return new ActivityPausedEventQuery(packageName);
    }

    /** {@link EventLogsQuery} for {@link ActivityPausedEvent}. */
    public static final class ActivityPausedEventQuery
            extends EventLogsQuery<ActivityPausedEvent, ActivityPausedEventQuery> {
        ActivityQueryHelper<ActivityPausedEventQuery> mActivity =
                new ActivityQueryHelper<>(this);

        private ActivityPausedEventQuery(String packageName) {
            super(ActivityPausedEvent.class, packageName);
        }

        /** Query {@link Activity}. */
        @CheckResult
        public ActivityQuery<ActivityPausedEventQuery> whereActivity() {
            return mActivity;
        }

        @Override
        protected boolean filter(ActivityPausedEvent event) {
            if (!mActivity.matches(event.mActivity)) {
                return false;
            }
            return true;
        }
    }

    /** Begin logging a {@link ActivityPausedEvent}. */
    public static ActivityPausedEventLogger logger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
        return new ActivityPausedEventLogger(activity, activityInfo);
    }

    /** {@link EventLogger} for {@link ActivityPausedEvent}. */
    public static final class ActivityPausedEventLogger
            extends EventLogger<ActivityPausedEvent> {
        private ActivityPausedEventLogger(Activity activity, android.content.pm.ActivityInfo activityInfo) {
            super(activity, new ActivityPausedEvent());
            setActivity(activityInfo);
        }

        /** Set the {@link Activity} being destroyed. */
        public ActivityPausedEventLogger setActivity(android.content.pm.ActivityInfo activity) {
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
        return "ActivityPausedEvent{"
                + ", activity=" + mActivity
                + ", packageName='" + mPackageName + "'"
                + ", timestamp=" + mTimestamp
                + "}";
    }
}
