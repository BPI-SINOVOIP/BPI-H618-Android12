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

import static com.google.common.truth.Truth.assertThat;

import android.app.Activity;

import com.android.activitycontext.ActivityContext;
import com.android.queryable.info.ActivityInfo;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ActivityInfoTest {

    private static final Class<? extends Activity> TEST_CLASS = ActivityContext.class;
    private static final String TEST_CLASS_NAME = ActivityContext.class.getName();
    private static final String TEST_CLASS_SIMPLE_NAME = ActivityContext.class.getSimpleName();

    @Test
    public void classConstructor_setsClass() {
        ActivityInfo activityInfo = ActivityInfo.builder().activityClass(TEST_CLASS).build();

        assertThat(activityInfo.className()).isEqualTo(TEST_CLASS_NAME);
    }

    @Test
    public void instanceConstructor_setsClass() throws Exception {
        ActivityInfo activityInfo = ActivityContext.getWithContext(c ->
                ActivityInfo.builder().activityClass(c).build());

        assertThat(activityInfo.className()).isEqualTo(TEST_CLASS_NAME);
    }

    @Test
    public void stringConstructor_setsClass() {
        ActivityInfo activityInfo = ActivityInfo.builder().activityClass(TEST_CLASS_NAME).build();

        assertThat(activityInfo.className()).isEqualTo(TEST_CLASS_NAME);
    }

    @Test
    public void activityInfoConstructor_setsClass() {
        android.content.pm.ActivityInfo frameworkActivityInfo =
                new android.content.pm.ActivityInfo();
        frameworkActivityInfo.name = TEST_CLASS_NAME;

        ActivityInfo activityInfo = ActivityInfo.builder(frameworkActivityInfo).build();

        assertThat(activityInfo.className()).isEqualTo(TEST_CLASS_NAME);
    }
}
