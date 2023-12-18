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

package com.android.bedstead.nene.activities;

import static android.Manifest.permission.REAL_GET_TASKS;

import android.app.ActivityManager;

import com.android.bedstead.nene.TestApis;
import com.android.bedstead.nene.annotations.Experimental;
import com.android.bedstead.nene.packages.ComponentReference;
import com.android.bedstead.nene.permissions.PermissionContext;

import java.util.List;

public final class Activities {

    private final TestApis mTestApis;

    public Activities(TestApis testApis) {
        mTestApis = testApis;
    }

    /**
     * Wrap the given {@link NeneActivity} to use Nene APIs.
     */
    public Activity<NeneActivity> wrap(NeneActivity activity) {
        return new Activity<>(mTestApis, activity, activity);
    }

    /**
     * Wrap the given {@link NeneActivity} subclass to use Nene APIs.
     */
    public <E extends NeneActivity> Activity<E> wrap(Class<E> clazz, E activity) {
        return new Activity<>(mTestApis, activity, activity);
    }

    /**
     * Wrap the given {@link android.app.Activity} to use Nene APIs.
     */
    public LocalActivity wrap(android.app.Activity activity) {
        return new LocalActivity(mTestApis, activity);
    }

    /**
     * Get the {@link ComponentReference} of the activity currently in the foreground.
     */
    @Experimental
    public ComponentReference foregroundActivity() {
        try (PermissionContext p = mTestApis.permissions().withPermission(REAL_GET_TASKS)) {
            ActivityManager activityManager =
                    mTestApis.context().instrumentedContext().getSystemService(
                            ActivityManager.class);
            List<ActivityManager.RunningTaskInfo> runningTasks = activityManager.getRunningTasks(1);
            if (runningTasks.isEmpty()) {
                return null;
            }

            return new ComponentReference(mTestApis, runningTasks.get(0).topActivity);
        }
    }

    /**
     * Return the current state of task locking. The three possible outcomes
     * are {@link ActivityManager#LOCK_TASK_MODE_NONE},
     * {@link ActivityManager#LOCK_TASK_MODE_LOCKED}
     * and {@link ActivityManager#LOCK_TASK_MODE_PINNED}.
     */
    @Experimental
    public int getLockTaskModeState() {
        ActivityManager activityManager =
                mTestApis.context().instrumentedContext().getSystemService(
                        ActivityManager.class);

        return activityManager.getLockTaskModeState();
    }
}
