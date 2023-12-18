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
package com.android.car.settings.enterprise;

import static android.car.test.mocks.AndroidMockitoHelper.syncCallOnMainThread;

import static com.google.common.truth.Truth.assertWithMessage;

import android.content.Context;
import android.content.res.XmlResourceParser;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Before;
import org.junit.Test;

public final class DeviceAdminAddFragmentTest extends BaseEnterpriseTestCase {

    private final Context mRealContext = ApplicationProvider.getApplicationContext();

    private DeviceAdminAddFragment mFragment;

    @Before
    public void createFragment() throws Exception {
        mFragment = syncCallOnMainThread(() -> new DeviceAdminAddFragment());
    }

    @Test
    public void testGetPreferenceScreenResId() {
        int resId = mFragment.getPreferenceScreenResId();

        XmlResourceParser parser = mRealContext.getResources().getXml(resId);
        assertWithMessage("xml with id%s", resId).that(parser).isNotNull();
    }

    // TODO(b/191269229): add tests for onAttach()
}
