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

package com.android.car.notification.template;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class CarNotificationBodyViewTest {
    private static final String TEST_TITLE = "TEST_TITLE";
    private static final String TEST_BODY = "TEST BODY";

    private CarNotificationBodyView mCarNotificationBodyView;
    private Context mContext;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        mContext = ApplicationProvider.getApplicationContext();
        mCarNotificationBodyView = new CarNotificationBodyView(mContext, /* attrs= */ null);
        mCarNotificationBodyView.onFinishInflate();
    }

    @Test
    public void onBind_titleTextSet() {
        mCarNotificationBodyView.bind(TEST_TITLE, TEST_BODY, /* icon= */ null);
        assertThat(mCarNotificationBodyView.getTitleView().getText()).isEqualTo(TEST_TITLE);
    }

    @Test
    public void onBind_contentTextSet() {
        mCarNotificationBodyView.bind(TEST_TITLE, TEST_BODY, /* icon= */ null);
        assertThat(mCarNotificationBodyView.getContentView().getText()).isEqualTo(TEST_BODY);
    }

    @Test
    public void onBindTitleAndMessage_titleTextSet() {
        mCarNotificationBodyView.bindTitleAndMessage(TEST_TITLE, TEST_BODY);
        assertThat(mCarNotificationBodyView.getTitleView().getText()).isEqualTo(TEST_TITLE);
    }

    @Test
    public void onBindTitleAndMessage_contentTextSet() {
        mCarNotificationBodyView.bindTitleAndMessage(TEST_TITLE, TEST_BODY);
        assertThat(mCarNotificationBodyView.getContentView().getText()).isEqualTo(TEST_BODY);
    }
}
