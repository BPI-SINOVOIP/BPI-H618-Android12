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

package com.android.queryable.info;

import android.app.Activity;


/**
 * Wrapper for information about an {@link Activity}.
 *
 * <p>This is used instead of {@link Activity} so that it can be easily serialized.
 */
public final class ActivityInfo extends ClassInfo {

    private final boolean mExported;

    public static Builder builder() {
        return new Builder();
    }

    public static Builder builder(android.content.pm.ActivityInfo activityInfo) {
        return builder()
                .activityClass(activityInfo.name)
                .exported(activityInfo.exported);
    }

    private ActivityInfo(String activityClass, boolean exported) {
        super(activityClass);
        mExported = exported;
    }

    public boolean exported() {
        return mExported;
    }


    @Override
    public String toString() {
        return "Activity{"
                + "class=" + super.toString()
                + ", exported=" + mExported
                + "}";
    }

    public static final class Builder {
        String mActivityClass;
        boolean mExported;

        public Builder activityClass(String activityClassName) {
            mActivityClass = activityClassName;
            return this;
        }

        public Builder activityClass(Activity activity) {
            return activityClass(activity.getClass());
        }

        public Builder activityClass(Class<? extends Activity> activityClass) {
            return activityClass(activityClass.getName());
        }

        public Builder exported(boolean exported) {
            mExported = exported;
            return this;
        }

        public ActivityInfo build() {
            return new ActivityInfo(
                    mActivityClass,
                    mExported
            );
        }
    }
}
